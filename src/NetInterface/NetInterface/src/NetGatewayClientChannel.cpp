#include "NetGatewayClientChannel.h"
#include "NetInterfaceMacro.h"
#include "NetGatewayClientChannelState.h"
#include "NetExecution.h"
#include "CStringManager/CStringManagerAPI.h"
#include "NetInterface.h"
#include "NetInterfaceCredit.h"
#include "NetInterfaceDirectory.h"
#include "NetInterfaceConnection.h"
#include "NetInterfaceTransport.h"
#include "NetInterfaceMessage.h"
#include <new>

/** 本通道全部期限使用同一单调时钟
@return 单调毫秒
*/
static uint64_t GatewayClientNow()
{
    return NetInterface::monotonicMilliseconds();
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
    if (m_state->m_phase != NET_GATEWAY_CLIENT_IDLE || listener == nullptr || address == nullptr || *address == 0 ||
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
    catch (const std::bad_alloc&)
    {
        result = NI_NO_MEMORY;
    }
    catch (...)
    {
        result = NI_INVALID_STATE;
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
    if (m_state->m_phase >= NET_GATEWAY_CLIENT_CLOSING || m_state->m_drainDeadline != 0)
    {
        return;
    }
    if (peer != m_state->m_linkPeer || identity.m_role != NI_LINK_NODE || m_state->m_phase != NET_GATEWAY_CLIENT_MANAGING)
    {
        m_state->fail(NI_PROTOCOL_ERROR);
        return;
    }
    m_state->m_phase = NET_GATEWAY_CLIENT_OPEN_PENDING;
    // 运输从本轮 poll 内回调时，下方会直接提交 OPEN。
    if (!m_state->m_polling) { NetExecution::progress(); }
}

bool NetGatewayClientChannel::onLinkControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (peer != m_state->m_linkPeer || m_state->m_phase >= NET_GATEWAY_CLIENT_CLOSING || m_state->m_drainDeadline != 0)
    {
        return true;
    }
    try
    {
        ProtoMessage message;

        if (frame.m_productId != 0 || frame.m_payload == nullptr || frame.m_payloadSize == 0 || frame.m_payloadSize > 16384 ||
            !message.from(frame.m_payload, frame.m_payloadSize))
        {
            m_state->fail(NI_PROTOCOL_ERROR);
            return true;
        }
        const Variant* channel = message.findValue(NI_GATEWAY_CHANNEL);
        if (frame.m_kind == NI_EVENT && frame.m_requestId == 0 &&
            (frame.m_messageType == NI_GATEWAY_CHANNEL_CLOSE || frame.m_messageType == NI_GATEWAY_CREDIT))
        {
            if (m_state->m_phase < NET_GATEWAY_CLIENT_HANDSHAKING ||
                channel == nullptr || channel->type() != Variant::UINT64 ||
                NetInterfaceMessage::decodeUint64(*channel) != 1)
            {
                m_state->fail(NI_PROTOCOL_ERROR);
                return true;
            }
            if (frame.m_messageType == NI_GATEWAY_CHANNEL_CLOSE)
            {
                m_state->acceptClose(message, GatewayClientNow());
                return true;
            }
            m_state->acceptCredit(message);
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
        if (!channel || channel->type() != Variant::UINT64 || NetInterfaceMessage::decodeUint64(*channel) != 1 ||
            NetInterfaceCredit::window(message, window) != NI_OK || window > m_state->m_credit.capacity() ||
            m_state->m_credit.configure(window) != NI_OK)
        {
            m_state->fail(NI_PROTOCOL_ERROR);
            return true;
        }
        const std::string hostname = CStringManager::Format("service.%u", static_cast<unsigned int>(m_state->m_product));
        if (hostname.empty())
        {
            m_state->fail(NI_NO_MEMORY);
            return true;
        }
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
        if (!m_state->m_polling) { NetExecution::progress(); }
    }
    catch (...)
    {
        m_state->fail(NI_NO_MEMORY);
    }
    return true;
}

bool NetGatewayClientChannel::onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    if (peer != m_state->m_linkPeer || m_state->m_phase >= NET_GATEWAY_CLIENT_CLOSING || m_state->m_drainDeadline != 0)
    {
        return true;
    }
    return m_state->acceptRecord(record);
}

void NetGatewayClientChannel::onLinkClosed(uint64_t peer, int32_t reason)
{
    if (peer == m_state->m_linkPeer)
    {
        m_state->endInput(reason, GatewayClientNow());
    }
}

void NetGatewayClientChannel::sendOpen()
{
    if (m_state->m_control.empty())
    {
        ProtoMessage open;
        open.setKeyValue(NI_GATEWAY_WINDOW, NetInterfaceMessage::encodeInteger(static_cast<int32_t>(m_state->m_credit.capacity())));
        open.setKeyValue(NI_GATEWAY_TARGET_PRODUCT, NetInterfaceMessage::encodeInteger(static_cast<uint64_t>(m_state->m_product)));
        open.setKeyValue(NI_GATEWAY_CHANNEL, NetInterfaceMessage::encodeInteger(static_cast<uint64_t>(1)));
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
        credit.setKeyValue(NI_GATEWAY_CHANNEL, NetInterfaceMessage::encodeInteger(static_cast<uint64_t>(1)));
        credit.setKeyValue(NI_GATEWAY_CREDIT_BYTES, NetInterfaceMessage::encodeInteger(static_cast<int32_t>(m_state->m_controlCredit)));
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
        if (m_state->m_credit.pending() != 0) { NetExecution::progress(); }
    }
    else if (sent != NI_LIMIT_EXCEEDED)
    {
        m_state->fail(sent);
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
        if (m_state->m_phase != NET_GATEWAY_CLIENT_CLOSING && m_state->m_drainDeadline == 0 && !m_state->m_transport->poll())
        {
            // 同一轮可能已收到显式关闭事件或末批业务记录。
            m_state->endInput(NI_OK, GatewayClientNow());
        }
        const uint64_t now = GatewayClientNow();
        if (m_state->m_phase < NET_GATEWAY_CLIENT_ACTIVE && m_state->m_drainDeadline == 0)
        {
            const uint32_t budget = m_state->m_timing.m_startupMs == 0 ?
                m_state->m_options.handshakeTimeoutMs * 3 : m_state->m_timing.m_startupMs;
            if (m_state->m_phase == NET_GATEWAY_CLIENT_HANDSHAKING && m_state->m_tlsDeadline == 0)
            {
                // 用本轮交给底层首次 progress 的同一个时间点启动握手时钟。
                const uint32_t timeout = m_state->m_options.handshakeTimeoutMs;
                m_state->m_tlsDeadline = now > UINT64_MAX - timeout ? UINT64_MAX : now + timeout;
            }
            if (m_state->m_tlsDeadline != 0 && m_state->m_tlsDeadline - m_state->m_startedAt < budget)
            {
                // 即使宿主错过两个期限，也由先到期的 TLS 握手交付其原始错误。
                NetExecution::deadline(m_state->m_tlsDeadline, now);
            }
            else
            {
                const uint64_t elapsed = now - m_state->m_startedAt;
                NetExecution::deadline(budget, elapsed);
                if (elapsed >= budget) { m_state->fail(NI_TIMEOUT); }
            }
        }
        if (m_state->m_phase == NET_GATEWAY_CLIENT_OPEN_PENDING)
        {
            sendOpen();
        }
        if (m_state->m_phase == NET_GATEWAY_CLIENT_HANDSHAKING || m_state->m_phase == NET_GATEWAY_CLIENT_ACTIVE)
        {
            m_state->advanceInput(now);
            if (m_state->m_phase < NET_GATEWAY_CLIENT_CLOSING && m_state->m_drainDeadline == 0)
            {
                sendCredit();
                m_state->sendOutput();
            }
        }
    }
    catch (const std::bad_alloc&)
    {
        m_state->fail(NI_NO_MEMORY);
    }
    catch (...)
    {
        m_state->fail(NI_INVALID_STATE);
    }
    finish();
    m_state->m_polling = false;
    return m_state->m_phase != NET_GATEWAY_CLIENT_CLOSED;
}

int32_t NetGatewayClientChannel::send(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (peer != 1 || m_state->m_phase != NET_GATEWAY_CLIENT_ACTIVE || m_state->m_drainDeadline != 0)
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
    if (m_state->m_polling)
    {
        m_state->fail(NI_OK);
        return;
    }
    m_state->m_polling = true;
    m_state->fail(NI_OK);
    finish();
    m_state->m_polling = false;
}

int32_t NetGatewayClientChannel::outputState(uint64_t peer) const
{
    if (peer != 1 || m_state->m_phase >= NET_GATEWAY_CLIENT_CLOSING || m_state->m_drainDeadline != 0 || !m_state->m_connection)
    {
        return NI_INVALID_STATE;
    }
    const int32_t local = m_state->m_connection->outputState();
    if (local != NI_OK)
    {
        return local;
    }
    if (m_state->m_credit.pending() != 0) { return NI_READY; }
    try
    {
        return m_state->m_transport->outputState(m_state->m_linkPeer);
    }
    catch (...)
    {
        return NI_INVALID_STATE;
    }
}