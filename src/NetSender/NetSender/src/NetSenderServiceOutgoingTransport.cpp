#include "NetSenderMacro.h"
#include "NetSenderServiceOutgoingTransport.h"
#include "NetSenderServiceGatewayState.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "ProtoMessageManager/ProtoMessageManagerAPI.h"

NetSenderServiceOutgoingTransport::NetSenderServiceOutgoingTransport(NetSenderServiceGateway& gateway) :
m_owner(gateway.m_state),
m_listener(nullptr),
m_channel(0),
m_request(0),
m_product(0),
m_phase(NET_SERVICE_OUTGOING_IDLE),
m_polling(false)
{

}

NetSenderServiceOutgoingTransport::~NetSenderServiceOutgoingTransport()
{
    shutdown();
}

int32_t NetSenderServiceOutgoingTransport::connect(const char* address, uint32_t port,
    const NetInterfaceLinkOptions& options, NetLinkTransportListener* listener, uint64_t& peer)
{
    peer = 0;
    if (m_phase != NET_SERVICE_OUTGOING_IDLE || listener == nullptr || address == nullptr || address[0] == 0 ||
        port == 0 || port > 65535 || options.m_version != 3 || options.m_role != NI_LINK_CLIENT || options.m_tls.server != 0)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (m_owner->m_phase != NET_SERVICE_READY || m_owner->m_outgoing != nullptr || m_owner->m_outgoingClose != 0)
    {
        return NI_INVALID_STATE;
    }
    if (m_owner->m_highestOutgoing >= UINT64_C(0x7ffffffffffffffe))
    {
        return NI_LIMIT_EXCEEDED;
    }
    m_channel = ++m_owner->m_highestOutgoing;
    m_request = m_channel + 1;
    m_owner->m_outgoing = this;
    m_listener = listener;
    m_phase = NET_SERVICE_OUTGOING_ATTACHED;
    peer = m_channel;
    return NI_OK;
}

bool NetSenderServiceOutgoingTransport::poll()
{
    if (m_listener == nullptr)
    {
        return false;
    }
    if (m_polling)
    {
        return !m_owner->closing();
    }
    m_polling = true;
    if (m_owner->closing())
    {
        closed(m_owner->m_reason);
    }
    else if (m_phase == NET_SERVICE_OUTGOING_ATTACHED)
    {
        // 先提交通知阶段，回调可同步提交OPEN或停止本通道。
        m_phase = NET_SERVICE_OUTGOING_ANNOUNCED;
        NetInterfaceLinkPeer identity = {};
        identity.m_role = NI_LINK_NODE;
        identity.m_name = m_owner->m_nodeName.c_str();
        identity.m_certificate = reinterpret_cast<const uint8_t*>(m_owner->m_nodeCertificate.data());
        identity.m_certificateSize = static_cast<uint32_t>(m_owner->m_nodeCertificate.size());
        try
        {
            m_listener->onLinkReady(m_channel, identity);
        }
        catch (...)
        {
            closed(NI_INVALID_STATE, true);
        }
    }
    m_polling = false;
    return m_listener != nullptr;
}

int32_t NetSenderServiceOutgoingTransport::sendControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (peer != m_channel || m_listener == nullptr || m_owner->closing() ||
        (m_phase != NET_SERVICE_OUTGOING_ANNOUNCED && m_phase != NET_SERVICE_OUTGOING_OPEN))
    {
        return NI_INVALID_STATE;
    }
    const bool opening = frame.m_kind == NI_REQUEST && frame.m_messageType == NI_GATEWAY_OPEN && frame.m_requestId == 1 &&
        m_phase == NET_SERVICE_OUTGOING_ANNOUNCED;
    const bool credit = frame.m_kind == NI_EVENT && frame.m_messageType == NI_GATEWAY_CREDIT && frame.m_requestId == 0 &&
        m_phase == NET_SERVICE_OUTGOING_OPEN;
    if ((!opening && !credit) || frame.m_productId != 0 || frame.m_payload == nullptr || frame.m_payloadSize == 0 || frame.m_payloadSize > 16384)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        ProtoMessage message;
        if (!message.from(std::string(reinterpret_cast<const char*>(frame.m_payload), frame.m_payloadSize)))
        {
            return NI_INVALID_ARGUMENT;
        }
        Variant channel;
        if (!message.getKeyValue(NI_GATEWAY_CHANNEL, channel) || channel.type() != Variant::UINT64 || static_cast<uint64_t>(channel) != 1)
        {
            return NI_INVALID_ARGUMENT;
        }
        uint32_t product = m_product;
        if (opening)
        {
            Variant target;
            if (!message.getKeyValue(NI_GATEWAY_TARGET_PRODUCT, target) || target.type() != Variant::UINT64 ||
                static_cast<uint64_t>(target) > UINT32_MAX)
            {
                return NI_INVALID_ARGUMENT;
            }
            product = static_cast<uint32_t>(static_cast<uint64_t>(target));
        }
        message.setKeyValue(NI_GATEWAY_CHANNEL, Variant(m_channel));
        std::string bytes;
        if (!message.toString(bytes))
        {
            return NI_PROTOCOL_ERROR;
        }
        NetInterfaceFrame mapped = frame;
        if (opening)
        {
            mapped.m_requestId = m_request;
        }
        mapped.m_payload = reinterpret_cast<const uint8_t*>(bytes.data());
        mapped.m_payloadSize = static_cast<uint32_t>(bytes.size());
        const int32_t result = m_owner->m_link.sendControl(m_owner->m_peer, mapped);
        if (result == NI_OK && opening)
        {
            m_product = product;
            m_phase = NET_SERVICE_OUTGOING_OPEN_WAIT;
        }
        return result;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetSenderServiceOutgoingTransport::sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    if (peer != m_channel || m_listener == nullptr || m_phase != NET_SERVICE_OUTGOING_OPEN || m_owner->closing())
    {
        return NI_INVALID_STATE;
    }
    if (record.m_channel != 1 || record.m_product != m_product || record.m_kind != NI_TUNNEL_BYTES ||
        record.m_data == nullptr || record.m_size == 0 || record.m_size > NET_INTERFACE_TUNNEL_PAYLOAD_LIMIT)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetInterfaceTunnelRecord mapped = record;
    mapped.m_channel = m_channel;
    return m_owner->m_link.sendRecord(m_owner->m_peer, mapped);
}

bool NetSenderServiceOutgoingTransport::control(const NetInterfaceFrame& frame)
{
    if (m_listener == nullptr)
    {
        return true;
    }
    const bool opening = frame.m_kind == NI_RESPONSE && frame.m_requestId == m_request && frame.m_messageType == NI_GATEWAY_OPEN &&
        m_phase == NET_SERVICE_OUTGOING_OPEN_WAIT;
    const bool control = frame.m_kind == NI_EVENT && frame.m_requestId == 0 && m_phase == NET_SERVICE_OUTGOING_OPEN &&
        (frame.m_messageType == NI_GATEWAY_CREDIT || frame.m_messageType == NI_GATEWAY_CHANNEL_CLOSE);
    if ((!opening && !control) || frame.m_productId != 0 || frame.m_payload == nullptr || frame.m_payloadSize == 0 || frame.m_payloadSize > 16384)
    {
        m_owner->fail(NI_PROTOCOL_ERROR);
        return true;
    }
    try
    {
        ProtoMessage message;
        if (!message.from(std::string(reinterpret_cast<const char*>(frame.m_payload), frame.m_payloadSize)))
        {
            m_owner->fail(NI_PROTOCOL_ERROR);
            return true;
        }
        const bool opened = opening && message.getCode() == NI_OK;
        if (!opening || opened)
        {
            Variant channel;
            if (!message.getKeyValue(NI_GATEWAY_CHANNEL, channel) || channel.type() != Variant::UINT64 || static_cast<uint64_t>(channel) != m_channel)
            {
                m_owner->fail(NI_PROTOCOL_ERROR);
                return true;
            }
            message.setKeyValue(NI_GATEWAY_CHANNEL, Variant(UINT64_C(1)));
        }
        std::string bytes;
        if (!message.toString(bytes))
        {
            m_owner->fail(NI_PROTOCOL_ERROR);
            return true;
        }
        NetInterfaceFrame mapped = frame;
        if (opening)
        {
            mapped.m_requestId = 1;
        }
        mapped.m_payload = reinterpret_cast<const uint8_t*>(bytes.data());
        mapped.m_payloadSize = static_cast<uint32_t>(bytes.size());
        bool accepted = false;
        try
        {
            accepted = m_listener->onLinkControl(m_channel, mapped);
        }
        catch (...)
        {
            closed(NI_INVALID_STATE, true);
            return true;
        }
        if (accepted && opened && m_phase == NET_SERVICE_OUTGOING_OPEN_WAIT)
        {
            m_phase = NET_SERVICE_OUTGOING_OPEN;
        }
        return accepted;
    }
    catch (...)
    {
        m_owner->fail(NI_NO_MEMORY);
        return true;
    }
}

bool NetSenderServiceOutgoingTransport::record(const NetInterfaceTunnelRecord& record)
{
    if (m_listener == nullptr)
    {
        return true;
    }
    if (m_phase != NET_SERVICE_OUTGOING_OPEN || record.m_channel != m_channel || record.m_product != m_product ||
        record.m_data == nullptr || record.m_size == 0 || record.m_size > NET_INTERFACE_TUNNEL_PAYLOAD_LIMIT ||
        (record.m_kind != NI_TUNNEL_BYTES && (record.m_kind != NI_TUNNEL_CLOSE || record.m_size != 4)))
    {
        m_owner->fail(NI_PROTOCOL_ERROR);
        return true;
    }
    if (record.m_kind == NI_TUNNEL_CLOSE)
    {
        closed(NI_TRUNCATED);
        return true;
    }
    NetInterfaceTunnelRecord mapped = record;
    mapped.m_channel = 1;
    try
    {
        return m_listener->onLinkRecord(m_channel, mapped);
    }
    catch (...)
    {
        closed(NI_INVALID_STATE, true);
        return true;
    }
}

void NetSenderServiceOutgoingTransport::detach(bool notifyRemote)
{
    const bool pending = m_phase == NET_SERVICE_OUTGOING_OPEN_WAIT || m_phase == NET_SERVICE_OUTGOING_OPEN;
    m_phase = NET_SERVICE_OUTGOING_CLOSED;
    m_listener = nullptr;
    if (m_owner->m_outgoing == this)
    {
        m_owner->m_outgoing = nullptr;
        if (notifyRemote && pending && !m_owner->closing())
        {
            m_owner->m_outgoingClose = m_channel;
            m_owner->m_outgoingCloseSince = 0;
        }
    }
}

void NetSenderServiceOutgoingTransport::closed(int32_t reason, bool notifyRemote)
{
    NetLinkTransportListener* listener = m_listener;
    detach(notifyRemote);
    if (listener != nullptr)
    {
        try
        {
            listener->onLinkClosed(m_channel, reason < 0 ? reason : NI_TRUNCATED);
        }
        catch (...)
        {
            // 终态已提交且借用已解除，回调失败不阻止所属服务继续回收。
        }
    }
}

void NetSenderServiceOutgoingTransport::shutdown()
{
    if (m_phase != NET_SERVICE_OUTGOING_IDLE)
    {
        detach(true);
    }
}

int32_t NetSenderServiceOutgoingTransport::outputState(uint64_t peer) const
{
    if (peer != m_channel || m_listener == nullptr || m_owner->closing() ||
        m_phase < NET_SERVICE_OUTGOING_ANNOUNCED || m_phase >= NET_SERVICE_OUTGOING_CLOSED)
    {
        return NI_INVALID_STATE;
    }
    return m_owner->m_link.outputState(m_owner->m_peer);
}