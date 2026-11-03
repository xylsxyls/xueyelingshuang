#include "NetGatewayClientChannel.h"
#include "NetInterfaceMacro.h"
#include "NetGatewayClientChannelState.h"
#include "NetInterface.h"
#include "NetInterfaceCredit.h"
#include "NetInterfaceDirectory.h"
#include "NetInterfaceConnection.h"
#include "NetInterfaceTransport.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include <algorithm>
#include <chrono>
#include <cstring>

/** 本通道全部期限使用同一单调时钟
@return 单调毫秒
*/
static uint64_t GatewayClientNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

NetGatewayClientChannel::NetGatewayClientChannel(uint32_t product, NetGatewayTransport* external) :
m_state(new NetGatewayClientChannelState(product, external))
{

}

NetGatewayClientChannel::~NetGatewayClientChannel()
{
    shutdown();
    delete m_state;
}

int32_t NetGatewayClientChannel::connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
    NetTransportListener* listener, uint64_t& peer)
{
    peer = 0;
    if (m_state->m_phase != NET_GATEWAY_CLIENT_IDLE || listener == nullptr || address == nullptr ||
        port == 0 || port > 65535 || options.server != 0 || options.entropy == nullptr ||
        options.handshakeTimeoutMs < 100 || options.handshakeTimeoutMs > 60000)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (m_state->m_transport == nullptr)
    {
        return NI_NOT_SUPPORTED;
    }
    m_state->m_options.entropy = options.entropy;
    m_state->m_options.entropyContext = options.entropyContext;
    m_state->m_options.handshakeTimeoutMs = options.handshakeTimeoutMs;
    m_state->m_listener = listener;
    m_state->m_phase = NET_GATEWAY_CLIENT_MANAGING;
    m_state->m_startedAt = GatewayClientNow();
    NetInterfaceLinkOptions management = {};
    management.m_version = 3;
    management.m_role = NI_LINK_CLIENT;
    management.m_tls = m_state->m_options;
    int32_t result = NI_NO_MEMORY;
    try
    {
        result = m_state->m_transport->connect(address, port, management, this, m_state->m_linkPeer);
        if (result == NI_OK && m_state->m_linkPeer == 0)
        {
            result = NI_PROTOCOL_ERROR;
        }
    }
    catch (...)
    {
        result = NI_NO_MEMORY;
    }
    if (result != NI_OK)
    {
        // 未接纳连接没有业务终态，但运输可能已经部分启动，仍统一停止
        m_state->m_listener = nullptr;
        m_state->fail(result);
        finish();
        return result;
    }
    peer = 1;
    return NI_OK;
}

void NetGatewayClientChannel::onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity)
{
    if (m_state->m_phase >= NET_GATEWAY_CLIENT_CLOSING)
    {
        return;
    }
    if (peer != m_state->m_linkPeer || identity.m_role != NI_LINK_NODE || m_state->m_phase != NET_GATEWAY_CLIENT_MANAGING)
    {
        m_state->fail(NI_PROTOCOL_ERROR);
        return;
    }
    m_state->m_phase = NET_GATEWAY_CLIENT_OPEN_PENDING;
}

bool NetGatewayClientChannel::onLinkControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (peer != m_state->m_linkPeer || m_state->m_phase >= NET_GATEWAY_CLIENT_CLOSING)
    {
        return true;
    }
    try
    {
        ProtoMessage message;
        Variant channel;
        if (frame.m_productId != 0 || frame.m_payload == nullptr || frame.m_payloadSize == 0 || frame.m_payloadSize > 16384 ||
            !message.from(std::string(reinterpret_cast<const char*>(frame.m_payload), frame.m_payloadSize)))
        {
            m_state->fail(NI_PROTOCOL_ERROR);
            return true;
        }
        if (frame.m_kind == NI_EVENT && frame.m_requestId == 0 &&
            (frame.m_messageType == NI_GATEWAY_CHANNEL_CLOSE || frame.m_messageType == NI_GATEWAY_CREDIT))
        {
            if (m_state->m_phase < NET_GATEWAY_CLIENT_HANDSHAKING ||
                !message.getKeyValue(NI_GATEWAY_CHANNEL, channel) || channel.type() != Variant::UINT64 || static_cast<uint64_t>(channel) != 1)
            {
                m_state->fail(NI_PROTOCOL_ERROR);
                return true;
            }
            if (frame.m_messageType == NI_GATEWAY_CHANNEL_CLOSE)
            {
                m_state->fail(NI_TRUNCATED);
                return true;
            }
            Variant amount;
            if (!message.getKeyValue(NI_GATEWAY_CREDIT_BYTES, amount) || amount.type() != Variant::INT32 ||
                static_cast<int32_t>(amount) <= 0 || m_state->m_credit.grant(static_cast<uint32_t>(static_cast<int32_t>(amount))) != NI_OK)
            {
                m_state->fail(NI_PROTOCOL_ERROR);
            }
            return true;
        }
        if (m_state->m_phase != NET_GATEWAY_CLIENT_OPEN_WAIT || frame.m_kind != NI_RESPONSE ||
            frame.m_requestId != 1 || frame.m_messageType != NI_GATEWAY_OPEN)
        {
            m_state->fail(NI_PROTOCOL_ERROR);
            return true;
        }
        if (message.getCode() != NI_OK)
        {
            m_state->fail(message.getCode() < 0 ? message.getCode() : NI_PROTOCOL_ERROR);
            return true;
        }
        uint32_t window = 0;
        if (!message.getKeyValue(NI_GATEWAY_CHANNEL, channel) || channel.type() != Variant::UINT64 || static_cast<uint64_t>(channel) != 1 ||
            NetInterfaceCredit::window(message, window) != NI_OK || m_state->m_credit.configure(window) != NI_OK)
        {
            m_state->fail(NI_PROTOCOL_ERROR);
            return true;
        }
        const std::string hostname = "service." + std::to_string(m_state->m_product);
        NetInterfaceTlsOptions tls = m_state->m_options;
        tls.authority = NetInterface::trustAuthority(&tls.authoritySize);
        tls.hostname = hostname.c_str();
        std::unique_ptr<NetInterfaceConnection> connection(new NetInterfaceConnection(tls, NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT + 32));
        if (connection->status() < 0)
        {
            m_state->fail(connection->status());
            return true;
        }
        m_state->m_connection.swap(connection);
        m_state->m_phase = NET_GATEWAY_CLIENT_HANDSHAKING;
    }
    catch (...)
    {
        m_state->fail(NI_NO_MEMORY);
    }
    return true;
}

bool NetGatewayClientChannel::onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    if (peer != m_state->m_linkPeer || m_state->m_phase >= NET_GATEWAY_CLIENT_CLOSING)
    {
        return true;
    }
    if (m_state->m_phase < NET_GATEWAY_CLIENT_HANDSHAKING || record.m_product != m_state->m_product ||
        record.m_channel != 1 || record.m_kind != NI_TUNNEL_BYTES || record.m_data == nullptr || record.m_size == 0 || record.m_size > sizeof(m_state->m_input))
    {
        m_state->fail(NI_PROTOCOL_ERROR);
        return true;
    }
    if (m_state->m_inputSize != 0)
    {
        return false;
    }
    if (m_state->m_credit.received(record.m_size) != NI_OK)
    {
        m_state->fail(NI_PROTOCOL_ERROR);
        return true;
    }
    uint32_t consumed = 0;
    const int32_t fed = m_state->m_connection->feed(record.m_data, record.m_size, consumed);
    if (fed < 0 || consumed > record.m_size)
    {
        m_state->fail(fed < 0 ? fed : NI_PROTOCOL_ERROR);
        return true;
    }
    if (consumed != record.m_size)
    {
        m_state->m_inputSize = record.m_size - consumed;
        m_state->m_inputOffset = 0;
        std::memcpy(m_state->m_input, record.m_data + consumed, m_state->m_inputSize);
    }
    return true;
}

void NetGatewayClientChannel::onLinkClosed(uint64_t peer, int32_t reason)
{
    if (peer == m_state->m_linkPeer)
    {
        m_state->fail(reason < 0 ? reason : NI_TRUNCATED);
    }
}

void NetGatewayClientChannel::sendOpen()
{
    if (m_state->m_control.empty())
    {
        ProtoMessage open;
        open.setKeyValue(NI_GATEWAY_WINDOW, Variant(static_cast<int32_t>(1048576)));
        open.setKeyValue(NI_GATEWAY_TARGET_PRODUCT, Variant(static_cast<uint64_t>(m_state->m_product)));
        open.setKeyValue(NI_GATEWAY_CHANNEL, Variant(static_cast<uint64_t>(1)));
        if (!open.toString(m_state->m_control))
        {
            m_state->fail(NI_NO_MEMORY);
            return;
        }
    }
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_REQUEST;
    frame.m_requestId = 1;
    frame.m_messageType = NI_GATEWAY_OPEN;
    frame.m_payload = reinterpret_cast<const uint8_t*>(m_state->m_control.data());
    frame.m_payloadSize = static_cast<uint32_t>(m_state->m_control.size());
    const int32_t sent = m_state->m_transport->sendControl(m_state->m_linkPeer, frame);
    if (sent == NI_OK && m_state->m_phase == NET_GATEWAY_CLIENT_OPEN_PENDING)
    {
        m_state->m_control.clear();
        m_state->m_phase = NET_GATEWAY_CLIENT_OPEN_WAIT;
    }
    else if (sent != NI_OK && sent != NI_LIMIT_EXCEEDED)
    {
        m_state->fail(sent);
    }
}

void NetGatewayClientChannel::sendCredit()
{
    if (m_state->m_control.empty())
    {
        m_state->m_controlCredit = m_state->m_credit.pending();
        if (m_state->m_controlCredit == 0)
        {
            return;
        }
        ProtoMessage credit;
        credit.setKeyValue(NI_GATEWAY_CHANNEL, Variant(static_cast<uint64_t>(1)));
        credit.setKeyValue(NI_GATEWAY_CREDIT_BYTES, Variant(static_cast<int32_t>(m_state->m_controlCredit)));
        if (!credit.toString(m_state->m_control))
        {
            m_state->fail(NI_NO_MEMORY);
            return;
        }
    }
    NetInterfaceFrame event = {};
    event.m_kind = NI_EVENT;
    event.m_messageType = NI_GATEWAY_CREDIT;
    event.m_payload = reinterpret_cast<const uint8_t*>(m_state->m_control.data());
    event.m_payloadSize = static_cast<uint32_t>(m_state->m_control.size());
    const int32_t sent = m_state->m_transport->sendControl(m_state->m_linkPeer, event);
    if (sent == NI_OK)
    {
        if (m_state->m_credit.consume(m_state->m_controlCredit) != NI_OK)
        {
            m_state->fail(NI_PROTOCOL_ERROR);
        }
        m_state->m_control.clear();
        m_state->m_controlCredit = 0;
    }
    else if (sent != NI_LIMIT_EXCEEDED)
    {
        m_state->fail(sent);
    }
}

void NetGatewayClientChannel::sendOutput()
{
    for (uint32_t step = 0; step < 32 && m_state->m_phase < NET_GATEWAY_CLIENT_CLOSING; ++step)
    {
        const uint8_t* bytes = nullptr;
        uint32_t size = 0;
        const int32_t pending = m_state->m_connection->peekOutput(bytes, size);
        if (pending < 0)
        {
            m_state->fail(pending);
            break;
        }
        if (size == 0 || m_state->m_credit.available() == 0)
        {
            break;
        }
        NetInterfaceTunnelRecord record = {};
        record.m_kind = NI_TUNNEL_BYTES;
        record.m_product = m_state->m_product;
        record.m_channel = 1;
        record.m_data = bytes;
        record.m_size = (std::min)((std::min)(size, static_cast<uint32_t>(16384)), m_state->m_credit.available());
        const int32_t sent = m_state->m_transport->sendRecord(m_state->m_linkPeer, record);
        if (sent == NI_LIMIT_EXCEEDED)
        {
            break;
        }
        if (sent != NI_OK)
        {
            m_state->fail(sent);
            break;
        }
        if (m_state->m_credit.sent(record.m_size) != NI_OK || m_state->m_connection->consumeOutput(record.m_size) < 0)
        {
            m_state->fail(NI_PROTOCOL_ERROR);
            break;
        }
    }
}

void NetGatewayClientChannel::advance(uint64_t now)
{
    if (m_state->m_inputSize != 0)
    {
        uint32_t consumed = 0;
        const uint32_t remaining = m_state->m_inputSize - m_state->m_inputOffset;
        const int32_t fed = m_state->m_connection->feed(m_state->m_input + m_state->m_inputOffset, remaining, consumed);
        if (fed < 0 || consumed > remaining)
        {
            m_state->fail(fed < 0 ? fed : NI_PROTOCOL_ERROR);
            return;
        }
        m_state->m_inputOffset += consumed;
        if (m_state->m_inputOffset == m_state->m_inputSize)
        {
            m_state->m_inputSize = m_state->m_inputOffset = 0;
        }
    }
    const int32_t status = m_state->m_connection->progress(now);
    if (status < 0 || status == NET_INTERFACE_TLS_CLOSED)
    {
        m_state->fail(status < 0 ? status : 0);
        return;
    }
    if (status == NET_INTERFACE_TLS_READY && m_state->m_phase == NET_GATEWAY_CLIENT_HANDSHAKING)
    {
        m_state->m_phase = NET_GATEWAY_CLIENT_ACTIVE;
        m_state->m_listener->onReady(1);
    }
    for (uint32_t step = 0; step < 32 && m_state->m_phase == NET_GATEWAY_CLIENT_ACTIVE; ++step)
    {
        NetInterfaceFrame frame = {};
        const int32_t available = m_state->m_connection->peek(frame);
        if (available < 0)
        {
            m_state->fail(available);
            break;
        }
        if (available != NI_READY)
        {
            break;
        }
        if (!m_state->m_listener->onFrame(1, frame))
        {
            if (m_state->m_blockedSince == 0)
            {
                m_state->m_blockedSince = now;
            }
            else if (now - m_state->m_blockedSince >= 5000)
            {
                m_state->fail(NI_TIMEOUT);
            }
            break;
        }
        m_state->m_blockedSince = 0;
        m_state->m_connection->consume();
    }
    if (m_state->m_phase < NET_GATEWAY_CLIENT_CLOSING)
    {
        sendCredit();
        sendOutput();
    }
}

void NetGatewayClientChannel::finish()
{
    if (m_state->m_phase != NET_GATEWAY_CLIENT_CLOSING)
    {
        return;
    }
    // 外部停止可同步交付旧事件；先封闭状态，旧事件不能覆盖首因或再次通知
    m_state->m_phase = NET_GATEWAY_CLIENT_CLOSED;
    NetTransportListener* listener = m_state->m_listener;
    m_state->m_listener = nullptr;
    try
    {
        m_state->m_transport->shutdown();
    }
    catch (...)
    {
        if (m_state->m_reason == NI_OK)
        {
            m_state->m_reason = NI_INVALID_STATE;
        }
    }
    m_state->m_connection.reset();
    m_state->m_inputSize = m_state->m_inputOffset = 0;
    m_state->m_control.clear();
    m_state->m_controlCredit = 0;
    if (listener != nullptr)
    {
        try
        {
            listener->onClosed(1, m_state->m_reason, m_state->m_detail);
        }
        catch (...)
        {
            // 唯一通知已经取走，观察者异常不能恢复通道或阻断poll保护复位
        }
    }
}

bool NetGatewayClientChannel::poll()
{
    if (m_state->m_phase == NET_GATEWAY_CLIENT_IDLE || m_state->m_phase == NET_GATEWAY_CLIENT_CLOSED)
    {
        return false;
    }
    if (m_state->m_polling)
    {
        return true;
    }
    m_state->m_polling = true;
    try
    {
        if (m_state->m_phase != NET_GATEWAY_CLIENT_CLOSING)
        {
            m_state->m_transport->poll();
        }
        const uint64_t now = GatewayClientNow();
        if (m_state->m_phase < NET_GATEWAY_CLIENT_ACTIVE &&
            now - m_state->m_startedAt >= static_cast<uint64_t>(m_state->m_options.handshakeTimeoutMs) * 3)
        {
            m_state->fail(NI_TIMEOUT);
        }
        if (m_state->m_phase == NET_GATEWAY_CLIENT_OPEN_PENDING)
        {
            sendOpen();
        }
        if (m_state->m_phase == NET_GATEWAY_CLIENT_HANDSHAKING || m_state->m_phase == NET_GATEWAY_CLIENT_ACTIVE)
        {
            advance(now);
        }
    }
    catch (...)
    {
        m_state->fail(NI_NO_MEMORY);
    }
    finish();
    m_state->m_polling = false;
    return m_state->m_phase != NET_GATEWAY_CLIENT_CLOSED;
}

int32_t NetGatewayClientChannel::send(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (peer != 1 || m_state->m_phase != NET_GATEWAY_CLIENT_ACTIVE)
    {
        return NI_INVALID_STATE;
    }
    if (m_state->m_product != 0 && frame.m_productId != 0 && frame.m_productId != m_state->m_product)
    {
        return NI_INVALID_ARGUMENT;
    }
    return m_state->m_connection->send(frame);
}

int32_t NetGatewayClientChannel::configureCompression(uint64_t peer, const uint32_t* types, uint32_t count, bool negotiated)
{
    if (peer != 1 || !m_state->m_connection || m_state->m_phase >= NET_GATEWAY_CLIENT_CLOSING)
    {
        return NI_INVALID_STATE;
    }
    return m_state->m_connection->configureCompression(types, count, negotiated);
}

void NetGatewayClientChannel::close(uint64_t peer, int32_t reason)
{
    if (peer == 1)
    {
        m_state->fail(reason);
    }
}

void NetGatewayClientChannel::shutdown()
{
    m_state->fail(NI_OK);
    if (!m_state->m_polling)
    {
        m_state->m_polling = true;
        finish();
        m_state->m_polling = false;
    }
}

int32_t NetGatewayClientChannel::outputState(uint64_t peer) const
{
    if (peer != 1 || m_state->m_phase >= NET_GATEWAY_CLIENT_CLOSING || !m_state->m_connection)
    {
        return NI_INVALID_STATE;
    }
    const int32_t local = m_state->m_connection->outputState();
    if (local != NI_OK)
    {
        return local;
    }
    try
    {
        return m_state->m_transport->outputState(m_state->m_linkPeer);
    }
    catch (...)
    {
        return NI_INVALID_STATE;
    }
}

// NET_GATEWAY_BUSINESS_UNIT_BEGIN
//#include "NetInterface.h"
//#include "NetInterface/NetClientSessionTest.h"
//#include "ProtoMessage/ProtoMessageAPI.h"
//#include <cstdio>
//#include <cstring>
//#include <stdexcept>
//#if defined(_MSC_VER) && defined(_DEBUG)
//#include <crtdbg.h>
//#endif
//
//NetGatewayChannelTestTransport::NetGatewayChannelTestTransport() :
//m_listener(nullptr), m_connectResult(NI_OK), m_zeroPeer(false), m_busy(false), m_pollThrow(false),
//m_stopThrow(false), m_closeDuringStop(false), m_announced(false), m_stops(0), m_controls(0), m_bytes(0), m_controlSize(0)
//{
//}
//
//int32_t NetGatewayChannelTestTransport::connect(const char* address, uint32_t port, const NetInterfaceLinkOptions& options,
//    NetLinkTransportListener* listener, uint64_t& peer)
//{
//    (void)address;
//    (void)port;
//    (void)options;
//    m_listener = listener;
//    peer = m_zeroPeer ? 0 : 7;
//    return m_connectResult;
//}
//
//bool NetGatewayChannelTestTransport::poll()
//{
//    if (m_pollThrow)
//    {
//        m_pollThrow = false;
//        throw 1;
//    }
//    if (!m_announced && m_listener != nullptr)
//    {
//        m_announced = true;
//        NetInterfaceLinkPeer identity = {};
//        identity.m_role = NI_LINK_NODE;
//        m_listener->onLinkReady(7, identity);
//    }
//    return m_listener != nullptr;
//}
//
//int32_t NetGatewayChannelTestTransport::sendControl(uint64_t peer, const NetInterfaceFrame& frame)
//{
//    if (peer != 7 || frame.m_payload == nullptr || frame.m_payloadSize > sizeof(m_control))
//    {
//        return NI_PROTOCOL_ERROR;
//    }
//    ++m_controls;
//    m_controlSize = frame.m_payloadSize;
//    std::memcpy(m_control, frame.m_payload, frame.m_payloadSize);
//    return m_busy ? NI_LIMIT_EXCEEDED : NI_OK;
//}
//
//int32_t NetGatewayChannelTestTransport::sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
//{
//    if (peer != 7 || record.m_channel != 1 || record.m_size == 0 || record.m_size > 16384 || record.m_data == nullptr)
//    {
//        return NI_PROTOCOL_ERROR;
//    }
//    m_bytes += record.m_size;
//    return NI_OK;
//}
//
//void NetGatewayChannelTestTransport::shutdown()
//{
//    ++m_stops;
//    NetLinkTransportListener* listener = m_listener;
//    m_listener = nullptr;
//    if (m_closeDuringStop && listener != nullptr)
//    {
//        listener->onLinkClosed(7, NI_TIMEOUT);
//    }
//    if (m_stopThrow)
//    {
//        m_stopThrow = false;
//        throw 2;
//    }
//}
//
//NetGatewayChannelTestListener::NetGatewayChannelTestListener() :
//m_channel(nullptr), m_closed(0), m_other(0), m_reason(0), m_throw(false)
//{
//}
//
//void NetGatewayChannelTestListener::onReady(uint64_t peer)
//{
//    (void)peer;
//    ++m_other;
//}
//
//bool NetGatewayChannelTestListener::onFrame(uint64_t peer, const NetInterfaceFrame& frame)
//{
//    (void)peer;
//    (void)frame;
//    ++m_other;
//    return true;
//}
//
//void NetGatewayChannelTestListener::onClosed(uint64_t peer, int32_t reason, int32_t detail)
//{
//    (void)peer;
//    (void)detail;
//    ++m_closed;
//    m_reason = reason;
//    if (m_channel != nullptr)
//    {
//        m_channel->close(1, NI_TIMEOUT);
//        m_channel->shutdown();
//        m_channel->poll();
//    }
//    if (m_throw)
//    {
//        throw 3;
//    }
//}
//
//int main()
//{
//    uint32_t checks = 0;
//    const auto Check = [&checks](bool pass, const char* message)
//    {
//        ++checks;
//        if (!pass) { throw std::runtime_error(message); }
//    };
//    NetInterfaceTlsOptions options = {};
//    options.handshakeTimeoutMs = 5000;
//    options.entropy = [](void*, uint8_t* bytes, uint32_t size) -> int32_t { std::memset(bytes, 0x41, size); return NI_OK; };
//    try
//    {
//        for (uint32_t mode = 0; mode < 4; ++mode)
//        {
//            NetGatewayChannelTestTransport transport;
//            NetGatewayChannelTestListener listener;
//            NetGatewayClientChannel channel(101, &transport);
//            listener.m_channel = &channel;
//            listener.m_throw = true;
//            transport.m_stopThrow = true;
//            transport.m_closeDuringStop = true;
//            transport.m_pollThrow = mode == 0;
//            transport.m_connectResult = mode == 2 ? NI_NOT_FOUND : NI_OK;
//            transport.m_zeroPeer = mode == 3;
//            uint64_t peer = 99;
//            const int32_t connected = channel.connect("127.0.0.1", 1, options, &listener, peer);
//            if (mode >= 2)
//            {
//                Check(connected == (mode == 2 ? NI_NOT_FOUND : NI_PROTOCOL_ERROR) && peer == 0 && listener.m_closed == 0 &&
//                    transport.m_stops == 1 && transport.m_listener == nullptr, "rejected connect cleanup/false terminal");
//            }
//            else
//            {
//                Check(connected == NI_OK && peer == 1, "accepted connect");
//                if (mode == 1) { channel.close(1, NI_CANCELLED); }
//                Check(!channel.poll() && listener.m_closed == 1 && listener.m_reason == (mode == 0 ? NI_NO_MEMORY : NI_CANCELLED),
//                    "shutdown exception lost terminal or first reason");
//            }
//            for (uint32_t index = 0; index < 1000; ++index)
//            {
//                Check(!channel.poll(), "closed poll revived");
//            }
//            channel.shutdown();
//            Check(transport.m_stops == 1 && listener.m_closed == (mode < 2 ? 1U : 0U) && listener.m_other == 0,
//                "duplicate stop/callback");
//        }
//        {
//            NetGatewayChannelTestTransport transport;
//            NetGatewayChannelTestListener listener;
//            NetGatewayClientChannel channel(101, &transport);
//            transport.m_busy = true;
//            uint64_t peer = 0;
//            Check(channel.connect("127.0.0.1", 1, options, &listener, peer) == NI_OK && channel.poll(), "busy OPEN setup");
//            uint8_t expected[256];
//            const uint32_t expectedSize = transport.m_controlSize;
//            std::memcpy(expected, transport.m_control, expectedSize);
//#if defined(_MSC_VER) && defined(_DEBUG)
//            static uint32_t s_allocations = 0;
//            const auto Hook = [](int operation, void*, size_t, int block, long, const unsigned char*, int) -> int
//            {
//                if (operation == _HOOK_ALLOC && block == _NORMAL_BLOCK) { ++s_allocations; }
//                return 1;
//            };
//            _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(Hook);
//#endif
//            bool unchanged = true;
//            for (uint32_t index = 0; index < 4096; ++index)
//            {
//                unchanged = channel.poll() && transport.m_controlSize == expectedSize &&
//                    std::memcmp(expected, transport.m_control, expectedSize) == 0 && unchanged;
//            }
//#if defined(_MSC_VER) && defined(_DEBUG)
//            _CrtSetAllocHook(previous);
//            Check(s_allocations == 0, "busy OPEN reallocated");
//#endif
//            Check(unchanged && transport.m_controls == 4097, "busy OPEN changed bytes/phase");
//            transport.m_busy = false;
//            Check(channel.poll() && transport.m_controls == 4098, "OPEN not accepted after backpressure");
//            Check(channel.poll() && transport.m_controls == 4098, "OPEN replayed after acceptance");
//            channel.shutdown();
//            Check(listener.m_closed == 1 && listener.m_reason == NI_OK, "normal close result");
//        }
//        for (uint32_t mode = 0; mode < 10; ++mode)
//        {
//            NetGatewayChannelTestTransport transport;
//            NetGatewayChannelTestListener listener;
//            NetGatewayClientChannel channel(101, &transport);
//            uint64_t peer = 0;
//            Check(channel.connect("127.0.0.1", 1, options, &listener, peer) == NI_OK && channel.poll(), "response setup");
//            ProtoMessage message;
//            message.setCode(NI_OK);
//            message.setKeyValue(NI_GATEWAY_CHANNEL, Variant(static_cast<uint64_t>(mode == 3 ? 2 : 1)));
//            message.setKeyValue(NI_GATEWAY_WINDOW, Variant(static_cast<int32_t>(mode == 4 ? 65537 : 65536)));
//            std::string body;
//            Check(message.toString(body), "response encoding");
//            NetInterfaceFrame frame = {};
//            frame.m_kind = NI_RESPONSE;
//            frame.m_messageType = NI_GATEWAY_OPEN;
//            frame.m_requestId = 1;
//            frame.m_payload = reinterpret_cast<const uint8_t*>(body.data());
//            frame.m_payloadSize = static_cast<uint32_t>(body.size());
//            if (mode == 0) { frame.m_productId = 99; }
//            if (mode == 1) { frame.m_requestId = 2; }
//            if (mode == 2) { frame.m_payload = nullptr; }
//            if (mode == 5) { frame.m_payloadSize = 16385; }
//            Check(transport.m_listener->onLinkControl(7, frame), "response consumption");
//            if (mode >= 6)
//            {
//                if (mode == 6)
//                {
//                    Check(transport.m_listener->onLinkControl(7, frame), "duplicate OPEN consumption");
//                }
//                else if (mode == 7)
//                {
//                    NetInterfaceTunnelRecord record = {};
//                    record.m_kind = NI_TUNNEL_BYTES;
//                    record.m_channel = 1;
//                    record.m_product = 101;
//                    record.m_data = reinterpret_cast<const uint8_t*>(body.data());
//                    record.m_size = 16385;
//                    Check(transport.m_listener->onLinkRecord(7, record), "oversized record consumption");
//                }
//                else
//                {
//                    message.setKeyValue(NI_GATEWAY_CREDIT_BYTES, Variant(static_cast<int32_t>(1)));
//                    if (mode == 9) { message.setKeyValue(NI_GATEWAY_CHANNEL, Variant(static_cast<uint64_t>(2))); }
//                    Check(message.toString(body), "event encoding");
//                    frame.m_kind = NI_EVENT;
//                    frame.m_requestId = 0;
//                    frame.m_messageType = mode == 8 ? NI_GATEWAY_CREDIT : NI_GATEWAY_CHANNEL_CLOSE;
//                    frame.m_payload = reinterpret_cast<const uint8_t*>(body.data());
//                    frame.m_payloadSize = static_cast<uint32_t>(body.size());
//                    Check(transport.m_listener->onLinkControl(7, frame), "event consumption");
//                }
//            }
//            Check(!channel.poll() && listener.m_closed == 1 && listener.m_reason == NI_PROTOCOL_ERROR && listener.m_other == 0,
//                "invalid control/record accepted");
//            channel.shutdown();
//        }
//        std::printf("Business channel lifecycle checks=%u PASS\n", checks);
//        return 0;
//    }
//    catch (const std::exception& error)
//    {
//        std::printf("Business channel checks=%u FAIL %s\n", checks, error.what());
//    }
//    catch (...)
//    {
//        std::printf("Business channel checks=%u FAIL escaped exception\n", checks);
//    }
//    return 1;
//}
// NET_GATEWAY_BUSINESS_UNIT_END