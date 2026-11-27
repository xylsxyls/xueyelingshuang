#include "NetGatewayClientChannelState.h"
#include "NetInterfaceMacro.h"
#include "NetInterfaceGatewayTypes.h"
#include "NetInterfaceMessage.h"
#include <algorithm>
#include <cstring>

NetGatewayClientChannelState::NetGatewayClientChannelState(uint32_t product, NetGatewayTransport* external) :
m_transport(external),
m_listener(nullptr),
m_options(),
m_inputSize(0),
m_inputOffset(0),
m_controlCredit(0),
m_linkPeer(0),
m_startedAt(0),
m_tlsDeadline(0),
m_blockedDeadline(0),
m_drainDeadline(0),
m_inputFinished(false),
m_product(product),
m_reason(0),
m_detail(0),
m_phase(NET_GATEWAY_CLIENT_IDLE),
m_polling(false)
{
    m_credit.configure(NET_CHANNEL_MAX_WINDOW);
}

void NetGatewayClientChannelState::fail(int32_t reason)
{
    if (m_phase == NET_GATEWAY_CLIENT_IDLE || m_phase >= NET_GATEWAY_CLIENT_CLOSING)
    {
        return;
    }
    m_phase = NET_GATEWAY_CLIENT_CLOSING;
    if (m_reason == NI_OK)
    {
        m_reason = m_drainDeadline != 0 && reason == NET_INTERFACE_TLS_TRUNCATED ? NI_TRUNCATED : reason;
    }
    m_detail = m_connection ? m_connection->detail() : 0;
    // 已接纳连接在两轮 poll 之间首次失败，下一轮还需停止运输并通知监听者。
    if (m_listener != nullptr && !m_polling) { NetExecution::progress(); }
}

void NetGatewayClientChannelState::acceptCredit(const ProtoMessage& message)
{
    const bool blocked = m_credit.available() == 0;
    const Variant* amount = message.findValue(NI_GATEWAY_CREDIT_BYTES);
    const int32_t bytes = amount && amount->type() == Variant::INT32 ? NetInterfaceMessage::decodeInt32(*amount) : 0;
    if (bytes <= 0 || m_credit.grant(static_cast<uint32_t>(bytes)) != NI_OK)
    {
        fail(NI_PROTOCOL_ERROR);
        return;
    }
    if (!m_polling && blocked && m_connection->outputState() == NI_READY) { NetExecution::progress(); }
}

void NetGatewayClientChannelState::acceptClose(const ProtoMessage& message, uint64_t now)
{
    const Variant* reason = message.findValue(NI_GATEWAY_REASON);
    if (reason == nullptr || reason->type() != Variant::INT32)
    {
        fail(NI_PROTOCOL_ERROR);
        return;
    }
    const int32_t code = NetInterfaceMessage::decodeInt32(*reason);
    if (code > 0) { fail(NI_PROTOCOL_ERROR); }
    else { endInput(code, now); }
}

void NetGatewayClientChannelState::endInput(int32_t reason, uint64_t now)
{
    if (m_phase >= NET_GATEWAY_CLIENT_CLOSING || m_drainDeadline != 0) { return; }
    if (!m_connection)
    {
        fail(reason < 0 ? reason : NI_TRUNCATED);
        return;
    }
    m_reason = reason < 0 ? reason : NI_OK;
    m_drainDeadline = UINT64_MAX - now < m_timing.m_backpressureMs ? UINT64_MAX : now + m_timing.m_backpressureMs;
    NetExecution::progress();
}

bool NetGatewayClientChannelState::acceptRecord(const NetInterfaceTunnelRecord& record)
{
    if (m_phase < NET_GATEWAY_CLIENT_HANDSHAKING || record.m_product != m_product ||
        record.m_channel != 1 || record.m_kind != NI_TUNNEL_BYTES || record.m_data == nullptr || record.m_size == 0 || record.m_size > sizeof(m_input))
    {
        fail(NI_PROTOCOL_ERROR);
        return true;
    }
    if (m_inputSize != 0)
    {
        return false;
    }
    if (m_credit.received(record.m_size) != NI_OK)
    {
        fail(NI_PROTOCOL_ERROR);
        return true;
    }
    uint32_t consumed = 0;
    const int32_t fed = m_connection->feed(record.m_data, record.m_size, consumed);
    if (fed < 0 || consumed > record.m_size)
    {
        fail(fed < 0 ? fed : NI_PROTOCOL_ERROR);
        return true;
    }
    if (consumed != record.m_size)
    {
        m_inputSize = record.m_size - consumed;
        m_inputOffset = 0;
        std::memcpy(m_input, record.m_data + consumed, m_inputSize);
        // 即使 TLS 本次零接纳，整条外层记录已由通道持有，还有 CREDIT 待返还。
        NetExecution::progress();
    }
    return true;
}

void NetGatewayClientChannelState::sendOutput()
{
    m_timing.writeRecords([&](uint32_t) -> bool {
        if (m_phase >= NET_GATEWAY_CLIENT_CLOSING || m_drainDeadline != 0) { return false; }
        const uint8_t* bytes = nullptr;
        uint32_t size = 0;
        const int32_t pending = m_connection->peekOutput(bytes, size);
        if (pending < 0)
        {
            fail(pending);
            return false;
        }
        if (size == 0 || m_credit.available() == 0)
        {
            return false;
        }
        NetInterfaceTunnelRecord record = {};
        record.m_kind = NI_TUNNEL_BYTES;
        record.m_product = m_product;
        record.m_channel = 1;
        record.m_data = bytes;
        record.m_size = (std::min)((std::min)(size, static_cast<uint32_t>(16384)), m_credit.available());
        const int32_t sent = m_transport->sendRecord(m_linkPeer, record);
        if (sent == NI_LIMIT_EXCEEDED)
        {
            return false;
        }
        if (sent != NI_OK)
        {
            fail(sent);
            return false;
        }
        if (m_credit.sent(record.m_size) != NI_OK || m_connection->consumeOutput(record.m_size) < 0)
        {
            fail(NI_PROTOCOL_ERROR);
            return false;
        }
        return true;
    });
}

void NetGatewayClientChannelState::advanceInput(uint64_t now)
{
    if (m_drainDeadline != 0)
    {
        if (now >= m_drainDeadline)
        {
            fail(m_blockedDeadline != 0 ? NI_TIMEOUT : NI_TRUNCATED);
            return;
        }
        NetExecution::deadline(m_drainDeadline, now);
        // 运输已结束。释放一段不能再投递的输出，让因 WANT_WRITE 停顿的 TLS 可以继续读取。
        const uint8_t* bytes = nullptr;
        uint32_t size = 0;
        if (m_connection->peekOutput(bytes, size) >= 0 && size != 0)
        {
            m_connection->consumeOutput(size);
        }
        if (m_inputSize == 0 && !m_inputFinished)
        {
            const int32_t ended = m_connection->finishInput();
            if (ended < 0) { fail(ended); return; }
            m_inputFinished = true;
        }
    }
    const int32_t status = m_connection->progress(now);
    if (status < 0 || status == NET_INTERFACE_TLS_CLOSED)
    {
        fail(status < 0 ? status : 0);
        return;
    }
    // 先让 TLS 消费已持有的密文，再搬运本通道暂存的后缀。
    if (m_inputSize != 0)
    {
        uint32_t consumed = 0;
        const uint32_t remaining = m_inputSize - m_inputOffset;
        const int32_t fed = m_connection->feed(m_input + m_inputOffset, remaining, consumed);
        if (fed < 0 || consumed > remaining)
        {
            fail(fed < 0 ? fed : NI_PROTOCOL_ERROR);
            return;
        }
        if (consumed != 0)
        {
            m_inputOffset += consumed;
            // 新入队的字节可能无需外部 I/O 就能继续处理。
            NetExecution::progress();
        }
        if (m_inputOffset == m_inputSize)
        {
            m_inputSize = m_inputOffset = 0;
        }
    }
    if (status == NET_INTERFACE_TLS_READY && m_phase == NET_GATEWAY_CLIENT_HANDSHAKING)
    {
        m_phase = NET_GATEWAY_CLIENT_ACTIVE;
        m_listener->onReady(1);
    }
    m_timing.readFrames([&](uint32_t step) -> bool {
        if (m_phase != NET_GATEWAY_CLIENT_ACTIVE) { return false; }
        if (step != 0)
        {
            const int32_t next = m_connection->progress(now);
            if (next < 0 || next == NET_INTERFACE_TLS_CLOSED)
            {
                fail(next < 0 ? next : NI_OK);
                return false;
            }
        }
        NetInterfaceFrame frame = {};
        const int32_t available = m_connection->peek(frame);
        if (available < 0)
        {
            fail(available);
            return false;
        }
        if (available != NI_READY)
        {
            return false;
        }
        const int32_t delivered = m_timing.deliver(*m_listener, frame, now, m_blockedDeadline);
        if (delivered != NI_OK)
        {
            if (delivered != NI_LIMIT_EXCEEDED) { fail(delivered); }
            return false;
        }
        m_connection->consume();
        return true;
    });
}