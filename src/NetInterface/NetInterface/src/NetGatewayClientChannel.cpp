#include "NetGatewayClientChannel.h"
#include "NetInterfaceMacro.h"
#include "NetGatewayClientChannelState.h"
#include "NetExecution.h"
#include "NetInterface.h"
#include "NetInterfaceCredit.h"
#include "NetInterfaceDirectory.h"
#include "NetInterfaceConnection.h"
#include "NetInterfaceTransport.h"
#include "ProtoMessageManager/ProtoMessageManagerAPI.h"
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

int32_t NetGatewayClientChannel::configureServicePollBudgets(uint32_t readFrames, uint32_t writeRecords)
{
    if (m_state->m_phase != NET_GATEWAY_CLIENT_IDLE) { return NI_INVALID_STATE; }
    return m_state->m_timing.configurePollBudgets(readFrames, writeRecords);
}

int32_t NetGatewayClientChannel::configureServiceTimeouts(uint32_t startupMs, uint32_t backpressureMs)
{
    if (m_state->m_phase != NET_GATEWAY_CLIENT_IDLE) { return NI_INVALID_STATE; }
    return m_state->m_timing.configure(startupMs, backpressureMs);
}

int32_t NetGatewayClientChannel::configureServiceWindow(uint32_t bytes)
{
    if (m_state->m_phase != NET_GATEWAY_CLIENT_IDLE) { return NI_INVALID_STATE; }
    return m_state->m_credit.configure(bytes);
}

int32_t NetGatewayClientChannel::configureTlsQueueLimits(const CEncodeDecodeTlsQueueLimits& limits)
{
    if (m_state->m_phase != NET_GATEWAY_CLIENT_IDLE) { return NI_INVALID_STATE; }
    m_state->m_ciphertext = limits;
    return NI_OK;
}

int32_t NetGatewayClientChannel::configureConnectionPools(const NetConnectionPoolConfig& config)
{
    if (m_state->m_phase != NET_GATEWAY_CLIENT_IDLE || m_state->m_polling)
    {
        return NI_INVALID_STATE;
    }
    if (!config.validCodecSettings()) { return NI_INVALID_ARGUMENT; }
    try
    {
        std::unique_ptr<NetConnectionPoolConfig> copy(new NetConnectionPoolConfig(config));
        m_state->m_connectionPools.swap(copy);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetGatewayClientChannel::connect(const char* address, uint32_t port, const NetInterfaceTlsOptions& options,
    NetTransportListener* listener, uint64_t& peer)
{
    peer = 0;
    if (m_state->m_phase != NET_GATEWAY_CLIENT_IDLE || listener == nullptr || address == nullptr ||
        port == 0 || port > 65535 || options.server != 0 || options.entropy == nullptr ||
        options.effectiveHandshakeTimeout() < 100 || options.handshakeTimeoutMs > 60000)
    {
        return NI_INVALID_ARGUMENT;
    }
    if (m_state->m_transport == nullptr)
    {
        return NI_NOT_SUPPORTED;
    }
    m_state->m_options.entropy = options.entropy;
    m_state->m_options.entropyContext = options.entropyContext;
    m_state->m_options.handshakeTimeoutMs = options.effectiveHandshakeTimeout();
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
            !message.from(frame.m_payload, frame.m_payloadSize))
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
            NetInterfaceCredit::window(message, window) != NI_OK || window > m_state->m_credit.capacity() ||
            m_state->m_credit.configure(window) != NI_OK)
        {
            m_state->fail(NI_PROTOCOL_ERROR);
            return true;
        }
        const std::string hostname = "service." + std::to_string(m_state->m_product);
        NetInterfaceTlsOptions tls = m_state->m_options;
        tls.authority = NetInterface::trustAuthority(&tls.authoritySize);
        tls.hostname = hostname.c_str();
        const uint32_t limit = NET_INTERFACE_DEFAULT_PAYLOAD_LIMIT + 32;
        std::unique_ptr<NetInterfaceConnection> connection(m_state->m_connectionPools ?
            new NetInterfaceConnection(tls, limit, *m_state->m_connectionPools, m_state->m_ciphertext) :
            new NetInterfaceConnection(tls, limit, NetConnectionPoolConfig(), m_state->m_ciphertext));
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
        open.setKeyValue(NI_GATEWAY_WINDOW, Variant(static_cast<int32_t>(m_state->m_credit.capacity())));
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
    m_state->m_timing.writeRecords([&](uint32_t) -> bool {
        if (m_state->m_phase >= NET_GATEWAY_CLIENT_CLOSING) { return false; }
        const uint8_t* bytes = nullptr;
        uint32_t size = 0;
        const int32_t pending = m_state->m_connection->peekOutput(bytes, size);
        if (pending < 0)
        {
            m_state->fail(pending);
            return false;
        }
        if (size == 0 || m_state->m_credit.available() == 0)
        {
            return false;
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
            return false;
        }
        if (sent != NI_OK)
        {
            m_state->fail(sent);
            return false;
        }
        if (m_state->m_credit.sent(record.m_size) != NI_OK || m_state->m_connection->consumeOutput(record.m_size) < 0)
        {
            m_state->fail(NI_PROTOCOL_ERROR);
            return false;
        }
        return true;
    });
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
    m_state->m_timing.readFrames([&](uint32_t step) -> bool {
        if (m_state->m_phase != NET_GATEWAY_CLIENT_ACTIVE) { return false; }
        if (step != 0)
        {
            const int32_t next = m_state->m_connection->progress(now);
            if (next < 0 || next == NET_INTERFACE_TLS_CLOSED)
            {
                m_state->fail(next < 0 ? next : NI_OK);
                return false;
            }
        }
        NetInterfaceFrame frame = {};
        const int32_t available = m_state->m_connection->peek(frame);
        if (available < 0)
        {
            m_state->fail(available);
            return false;
        }
        if (available != NI_READY)
        {
            return false;
        }
        const int32_t delivered = m_state->m_timing.deliver(*m_state->m_listener, frame, now, m_state->m_blockedDeadline);
        if (delivered != NI_OK)
        {
            if (delivered != NI_LIMIT_EXCEEDED) { m_state->fail(delivered); }
            return false;
        }
        m_state->m_connection->consume();
        return true;
    });
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
        if (m_state->m_phase < NET_GATEWAY_CLIENT_ACTIVE)
        {
            const uint32_t budget = m_state->m_timing.m_startupMs == 0 ?
                m_state->m_options.handshakeTimeoutMs * 3 : m_state->m_timing.m_startupMs;
            const uint64_t elapsed = now - m_state->m_startedAt;
            NetExecution::deadline(budget, elapsed);
            if (elapsed >= budget) { m_state->fail(NI_TIMEOUT); }
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

int32_t NetGatewayClientChannel::configureCompact(uint64_t peer, bool enabled)
{
    if (peer != 1 || !m_state->m_connection || m_state->m_phase >= NET_GATEWAY_CLIENT_CLOSING)
    {
        return NI_INVALID_STATE;
    }
    m_state->m_connection->configureCompact(enabled);
    return NI_OK;
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