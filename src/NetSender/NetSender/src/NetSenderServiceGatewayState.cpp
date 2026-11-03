#include "NetSenderServiceGatewayState.h"
#include "NetSenderServiceGatewayHelper.h"
#include "NetSenderMacro.h"
#include "CSystem/CSystemAPI.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "NetSenderServiceOutgoingTransport.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <sstream>

/** Use the reserved management budget so a slow user's data cannot block its close.
@param [in,out] link Authenticated service link
@param [in] peer Link generation
@param [in] channel Existing channel owned by this link
@param [in] reason Terminal reason
@return NI_OK, bounded backpressure or link error
*/
static int32_t ServiceGatewayClose(NetSenderLink& link, uint64_t peer, uint64_t channel, int32_t reason)
{
    ProtoMessage message;
    message.setKeyValue(NI_GATEWAY_CHANNEL, Variant(channel));
    message.setKeyValue(NI_GATEWAY_REASON, Variant(reason));
    std::string bytes;
    if (!message.toString(bytes)) { return NI_PROTOCOL_ERROR; }
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_EVENT;
    frame.m_messageType = NI_GATEWAY_CHANNEL_CLOSE;
    frame.m_payload = reinterpret_cast<const uint8_t*>(bytes.data());
    frame.m_payloadSize = static_cast<uint32_t>(bytes.size());
    return link.sendControl(peer, frame);
}

NetSenderServiceGatewayClient::NetSenderServiceGatewayClient(const NetInterfaceTlsOptions& tls, uint32_t role) :
m_connection(tls, NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT + 32),
m_origin(),
m_lastReceive(NetSenderServiceGatewayHelper::serviceGatewayNow()),
m_blockedSince(0),
m_closeSince(0),
m_role(role),
m_reason(0),
m_ready(false),
m_closing(false),
m_notifyRemote(false)
{

}

NetSenderServiceGatewayState::NetSenderServiceGatewayState(NetSenderLocalInbox& inbox) :
m_link(inbox),
m_listener(nullptr),
m_outgoing(nullptr),
m_highestOutgoing(0),
m_outgoingClose(0),
m_outgoingCloseSince(0),
m_peer(0),
m_highestClient(0),
m_product(0),
m_limit(0),
m_channels(0),
m_weight(0),
m_reason(0),
m_phase(NET_SERVICE_IDLE),
m_polling(false)
{

}

NetSenderServiceGatewayState::~NetSenderServiceGatewayState()
{
    if (!m_key.empty()) { NetInterface::wipe(reinterpret_cast<uint8_t*>(&m_key[0]), static_cast<uint32_t>(m_key.size())); }
}

void NetSenderServiceGatewayState::close(uint64_t id, int32_t reason, bool remote)
{
    auto found = m_clients.find(id);
    if (found == m_clients.end() || found->second->m_closing) { return; }
    found->second->m_closing = true;
    found->second->m_notifyRemote = remote;
    found->second->m_reason = reason;
    found->second->m_closeSince = NetSenderServiceGatewayHelper::serviceGatewayNow();
}

void NetSenderServiceGatewayState::fail(int32_t reason)
{
    if (closing()) { return; }
    m_phase = NET_SERVICE_CLOSING;
    m_reason = reason;
    m_link.close(m_peer, reason);
    for (auto& item : m_clients) { close(item.first, reason, false); }
}

void NetSenderServiceGatewayState::pump(uint64_t id, NetSenderServiceGatewayClient& client, uint64_t now)
{
    if (client.m_closing || closing()) { return; }
    if (!client.m_input.empty())
    {
        uint32_t consumed = 0;
        const int32_t fed = client.m_connection.feed(client.m_input.data(), static_cast<uint32_t>(client.m_input.size()), consumed);
        if (fed < 0) { close(id, fed); return; }
        client.m_input.erase(client.m_input.begin(), client.m_input.begin() + consumed);
    }
    const int32_t progressed = client.m_connection.progress(now);
    if (progressed < 0 || progressed == NET_INTERFACE_TLS_CLOSED) { close(id, progressed < 0 ? progressed : 0); return; }
    if (progressed == NET_INTERFACE_TLS_READY && !client.m_handshake)
    {
        NetInterfaceHandshakeOptions options = {};
        options.m_server = 1;
        options.m_capabilities = 15;
        options.m_maxRequests = m_limit;
        options.m_maxChunk = 65536;
        options.m_timeoutMs = 5000;
        if (NetSenderServiceGatewayHelper::serviceGatewayEntropy(nullptr, options.m_nonce, sizeof(options.m_nonce)) != 0) { close(id, NI_INVALID_STATE); return; }
        client.m_handshake.reset(new NetInterfaceHandshake(options, now));
    }
    if (client.m_handshake)
    {
        if (client.m_handshake->progress(now) < 0) { close(id, NI_TIMEOUT); return; }
        NetInterfaceFrame hello = {};
        if (client.m_handshake->peekOutput(hello) == NI_READY)
        {
            const int32_t sent = client.m_connection.send(hello);
            if (sent == NI_OK) { client.m_handshake->consumeOutput(); }
            else if (sent != NI_LIMIT_EXCEEDED) { close(id, sent); return; }
        }
        if (!client.m_ready && client.m_handshake->progress(now) == NI_READY)
        {
            uint32_t fields[4] = {};
            uint8_t nonce[16] = {};
            if (client.m_handshake->peerInfo(fields, nonce, client.m_origin.m_device) != NI_READY ||
                NetSenderServiceGatewayHelper::serviceGatewayEntropy(nullptr, reinterpret_cast<uint8_t*>(&client.m_origin.m_endpoint), sizeof(uint64_t)) != 0 ||
                client.m_origin.m_endpoint == 0)
            {
                close(id, NI_INVALID_STATE);
                return;
            }
            client.m_origin.m_platform = fields[0];
            if (client.m_connection.configureCompression(m_compressionTypes.empty() ? nullptr : m_compressionTypes.data(),
                static_cast<uint32_t>(m_compressionTypes.size()), (fields[1] & 8) != 0) != NI_OK)
            {
                close(id, NI_NO_MEMORY);
                return;
            }
            client.m_ready = true;
            m_listener->onGatewayClientReady(id, client.m_role, client.m_origin);
        }
        for (uint32_t step = 0; step < 32 && !client.m_closing && !closing(); ++step)
        {
            NetInterfaceFrame frame = {};
            const int32_t available = client.m_connection.peek(frame);
            if (available < 0) { close(id, available); return; }
            if (available != NI_READY) { break; }
            const int32_t accepted = client.m_handshake->accept(frame, now);
            if (accepted < 0) { close(id, accepted); return; }
            if (accepted != NI_OK)
            {
                client.m_connection.consume();
                break;
            }
            if (!client.m_ready) { close(id, NI_PROTOCOL_ERROR); return; }
            if (frame.m_kind == NI_PING || frame.m_kind == NI_PONG)
            {
                NetInterfaceFrame pong = {};
                const int32_t control = NetInterfaceRpc::control(frame, pong);
                if (control < 0) { close(id, control); return; }
                if (control == NI_READY)
                {
                    const int32_t sent = client.m_connection.send(pong);
                    if (sent == NI_LIMIT_EXCEEDED) { break; }
                    if (sent != NI_OK) { close(id, sent); return; }
                }
            }
            else
            {
                if (frame.m_productId == 0 || (m_product != 0 && frame.m_productId != m_product))
                {
                    close(id, NI_PROTOCOL_ERROR);
                    return;
                }
                if (!m_listener->onGatewayClientFrame(id, client.m_role, frame, client.m_origin))
                {
                    if (client.m_blockedSince == 0) { client.m_blockedSince = now; }
                    else if (now - client.m_blockedSince >= 5000) { close(id, NI_TIMEOUT); }
                    break;
                }
                client.m_blockedSince = 0;
            }
            client.m_connection.consume();
            client.m_lastReceive = now;
        }
    }
    if (client.m_input.empty() && client.m_credit.pending() != 0 && !client.m_closing && !closing())
    {
        const uint32_t amount = client.m_credit.pending();
        ProtoMessage message;
        message.setKeyValue(NI_GATEWAY_CHANNEL, Variant(id));
        message.setKeyValue(NI_GATEWAY_CREDIT_BYTES, Variant(static_cast<int32_t>(amount)));
        std::string bytes;
        if (!message.toString(bytes)) { close(id, NI_PROTOCOL_ERROR); return; }
        NetInterfaceFrame credit = {};
        credit.m_kind = NI_EVENT;
        credit.m_messageType = NI_GATEWAY_CREDIT;
        credit.m_payload = reinterpret_cast<const uint8_t*>(bytes.data());
        credit.m_payloadSize = static_cast<uint32_t>(bytes.size());
        const int32_t sent = m_link.sendControl(m_peer, credit);
        if (sent == NI_OK) { client.m_credit.consume(amount); }
        else if (sent != NI_LIMIT_EXCEEDED) { fail(sent); return; }
    }
    for (uint32_t step = 0; step < 32 && !client.m_closing && !closing(); ++step)
    {
        const uint8_t* bytes = nullptr;
        uint32_t size = 0;
        const int32_t pending = client.m_connection.peekOutput(bytes, size);
        if (pending < 0) { close(id, pending); return; }
        if (size == 0 || client.m_credit.available() == 0) { break; }
        NetInterfaceTunnelRecord record = {};
        record.m_kind = NI_TUNNEL_BYTES;
        record.m_product = m_product;
        record.m_channel = id;
        record.m_data = bytes;
        record.m_size = (std::min)((std::min)(size, static_cast<uint32_t>(16384)), client.m_credit.available());
        const int32_t sent = m_link.sendRecord(m_peer, record);
        if (sent == NI_LIMIT_EXCEEDED) { break; }
        if (sent != NI_OK) { fail(sent); return; }
        if (client.m_credit.sent(record.m_size) != NI_OK) { close(id, NI_PROTOCOL_ERROR); return; }
        if (client.m_connection.consumeOutput(record.m_size) < 0) { close(id, NI_INVALID_STATE); return; }
    }
    if (now >= client.m_lastReceive && now - client.m_lastReceive >= 45000) { close(id, NI_TIMEOUT); }
}

bool NetSenderServiceGatewayState::closing() const
{
    return m_phase == NET_SERVICE_CLOSING || m_phase == NET_SERVICE_CLOSED;
}

bool NetSenderServiceGatewayState::poll()
{
    if (m_phase == NET_SERVICE_IDLE || m_phase == NET_SERVICE_CLOSED)
    {
        return false;
    }
    if (m_polling)
    {
        return !closing();
    }
    m_polling = true;
    const uint64_t now = NetSenderServiceGatewayHelper::serviceGatewayNow();
    try
    {
        advance(now);
    }
    catch (...)
    {
        fail(NI_NO_MEMORY);
    }
    // 关闭记录编码失败后也走本轮无分配回收，不能带着polling标志逃逸。
    collectClosed(NetSenderServiceGatewayHelper::serviceGatewayNow());
    if (m_phase == NET_SERVICE_CLOSING && m_clients.empty())
    {
        try
        {
            if (m_outgoing != nullptr)
            {
                m_outgoing->poll();
            }
        }
        catch (...)
        {
            // 出站运输在回调前已解除监听和拥有者借用，保留首次终止原因。
        }
        m_link.shutdown();
        if (!m_key.empty())
        {
            NetInterface::wipe(reinterpret_cast<uint8_t*>(&m_key[0]), static_cast<uint32_t>(m_key.size()));
            m_key.clear();
        }
        m_phase = NET_SERVICE_CLOSED;
        if (m_listener != nullptr)
        {
            try
            {
                m_listener->onGatewayServiceClosed(m_reason);
            }
            catch (...)
            {
                // 唯一终态已经提交，业务回调失败不复活代次或阻止poll退出。
            }
        }
    }
    m_polling = false;
    return m_phase != NET_SERVICE_CLOSED;
}

void NetSenderServiceGatewayState::advance(uint64_t now)
{
    m_link.poll();
    // 接收回调会采集更新的活动时间；协议和期限推进必须使用回调结束后的时刻。
    now = NetSenderServiceGatewayHelper::serviceGatewayNow();
    if (m_phase == NET_SERVICE_AUTHENTICATED)
    {
        ProtoMessage ready;
        ready.setKeyValue(NI_GATEWAY_WINDOW, Variant(static_cast<int32_t>(1048576)));
        ready.setKeyValue(NI_GATEWAY_CAPACITY, Variant(static_cast<int32_t>(m_channels)));
        ready.setKeyValue(NI_GATEWAY_WEIGHT, Variant(static_cast<int32_t>(m_weight)));
        std::string bytes;
        NetInterfaceFrame frame = {};
        frame.m_kind = NI_REQUEST;
        frame.m_requestId = 1;
        frame.m_messageType = NI_GATEWAY_SERVICE_READY;
        if (!ready.toString(bytes)) { fail(NI_PROTOCOL_ERROR); }
        else
        {
            frame.m_payload = reinterpret_cast<const uint8_t*>(bytes.data());
            frame.m_payloadSize = static_cast<uint32_t>(bytes.size());
            const int32_t sent = m_link.sendControl(m_peer, frame);
            if (sent == NI_OK) { m_phase = NET_SERVICE_REGISTERING; }
            else if (sent != NI_LIMIT_EXCEEDED) { fail(sent); }
        }
    }
    if (m_outgoingClose != 0 && !closing())
    {
        if (m_outgoingCloseSince == 0) { m_outgoingCloseSince = now; }
        const int32_t sent = ServiceGatewayClose(m_link, m_peer, m_outgoingClose, NI_TRUNCATED);
        if (sent == NI_OK) { m_outgoingClose = 0; }
        else if (sent != NI_LIMIT_EXCEEDED || now - m_outgoingCloseSince >= 2000) { fail(sent); }
    }
    for (auto& item : m_clients)
    {
        try { pump(item.first, *item.second, now); }
        catch (...) { close(item.first, NI_NO_MEMORY); }
    }
}

void NetSenderServiceGatewayState::collectClosed(uint64_t now)
{
    for (auto item = m_clients.begin(); item != m_clients.end();)
    {
        NetSenderServiceGatewayClient& client = *item->second;
        if (!client.m_closing) { ++item; continue; }
        if (client.m_notifyRemote && !closing())
        {
            int32_t sent = NI_NO_MEMORY;
            try
            {
                sent = ServiceGatewayClose(m_link, m_peer, item->first, client.m_reason);
            }
            catch (...)
            {
                fail(NI_NO_MEMORY);
            }
            if (sent == NI_LIMIT_EXCEEDED && (now < client.m_closeSince || now - client.m_closeSince < 2000)) { ++item; continue; }
            if (sent != NI_OK) { fail(sent); }
        }
        const uint64_t id = item->first;
        const int32_t reason = client.m_reason;
        item = m_clients.erase(item);
        try { m_listener->onGatewayClientClosed(id, reason); }
        catch (...) { fail(NI_INVALID_STATE); }
        if (closing())
        {
            // 回调可能刚关闭此前已经略过的活动客户端；本轮收敛后才能析构管理链接。
            item = m_clients.begin();
        }
    }
}