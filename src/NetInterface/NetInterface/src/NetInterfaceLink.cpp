#include "NetInterfaceMacro.h"
#include "NetInterfaceLinkState.h"
#include "NetInterface.h"
#include "NetInterfaceLink.h"
#include "NetExecution.h"
#include "NetInterfaceTunnel.h"
#include <algorithm>
#include <cstring>

NetInterfaceLinkPools::NetInterfaceLinkPools() :
m_pingIntervalMs(15000),
m_pongTimeoutMs(10000),
m_drainTimeoutMs(5000),
m_controlStepBudget(64)
{
    m_outputRecords.initialCapacity = 128;
    m_outputRecords.maximumCapacity = 8388608 / sizeof(LockFreeHashMapPoolSlot<NetObjectPoolItem<NetInterfaceLinkWrite>>);
    m_outputRecords.directoryCapacity = 128;
    m_outputRecords.lookupCacheCapacity = 0;
}

NetInterfaceLink::NetInterfaceLink(const NetInterfaceLinkOptions& options) :
NetInterfaceLink(options, NetInterfaceLinkPools())
{

}

NetInterfaceLink::NetInterfaceLink(const NetInterfaceLinkOptions& options, const NetInterfaceLinkPools& pools) :
m_state(new NetInterfaceLinkState(pools))
{
    try
    {
        m_state->configure(options, pools);
    }
    catch (...)
    {
        m_state->fail(NI_NO_MEMORY);
    }
}

NetInterfaceLink::~NetInterfaceLink()
{
    delete m_state;
}

int32_t NetInterfaceLink::configureOutputPools(const NetBufferPoolConfig& bodies, const LockFreeHashMapPoolConfig& records)
{
    if (m_state->m_clockStarted || m_state->m_sentHello || m_state->m_receivedHello ||
        m_state->m_outputPeeked || m_state->m_outputHead != nullptr || m_state->status() < 0)
    {
        return NI_INVALID_STATE;
    }
    try
    {
        std::unique_ptr<NetBufferPool> newBodies(new NetBufferPool(bodies));
        if (newBodies->status() != NI_OK)
        {
            return newBodies->status();
        }
        std::unique_ptr<NetObjectPool<NetInterfaceLinkWrite>> newRecords(new NetObjectPool<NetInterfaceLinkWrite>(records));
        if (newRecords->status() != NI_OK)
        {
            return newRecords->status();
        }
        m_state->m_outputBodies.swap(newBodies);
        m_state->m_outputRecords.swap(newRecords);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetInterfaceLink::accept(const NetInterfaceTunnelRecord& record)
{
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    if (m_state->m_phase == NET_LINK_DRAINING)
    {
        return NI_INVALID_STATE;
    }
    uint32_t required = 0;
    if (NetInterfaceTunnel::encode(record, nullptr, 0, required) != NI_BUFFER_TOO_SMALL)
    {
        return m_state->fail(NI_PROTOCOL_ERROR);
    }
    if (record.m_kind == NI_TUNNEL_HELLO)
    {
        if (m_state->m_receivedHello)
        {
            return m_state->fail(NI_PROTOCOL_ERROR);
        }
        m_state->m_receivedHello = true;
        return NI_OK;
    }
    if (!m_state->m_receivedHello)
    {
        return m_state->fail(NI_PROTOCOL_ERROR);
    }
    try
    {
        if (record.m_kind == NI_TUNNEL_CONTROL)
        {
            if (!m_state->m_input.empty())
            {
                return NI_LIMIT_EXCEEDED;
            }
            m_state->m_input.assign(record.m_data, record.m_data + record.m_size);
            m_state->m_inputOffset = 0;
            return NI_OK;
        }
        if (m_state->status() != NI_READY)
        {
            return m_state->fail(NI_PROTOCOL_ERROR);
        }
        if (m_state->m_recordReady)
        {
            return NI_LIMIT_EXCEEDED;
        }
        m_state->m_receivedBytes.assign(record.m_data, record.m_data + record.m_size);
        m_state->m_received = record;
        m_state->m_received.m_data = m_state->m_receivedBytes.data();
        m_state->m_recordReady = true;
        return NI_OK;
    }
    catch (...)
    {
        return m_state->fail(NI_NO_MEMORY);
    }
}

int32_t NetInterfaceLink::progress(uint64_t now)
{
    try
    {
        return m_state->progress(now);
    }
    catch (...)
    {
        return m_state->fail(NI_NO_MEMORY);
    }
}

int32_t NetInterfaceLink::peekOutput(NetInterfaceTunnelRecord& record)
{
    record = NetInterfaceTunnelRecord();
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    if (m_state->m_phase == NET_LINK_DRAINING)
    {
        return NI_INVALID_STATE;
    }
    if (!m_state->m_outputPeeked)
    {
        NetInterfaceTunnelRecord next = {};
        if (!m_state->m_sentHello)
        {
            next.m_kind = NI_TUNNEL_HELLO;
        }
        else
        {
            if (!m_state->m_clockStarted)
            {
                // peek没有时钟参数；首个明确的progress负责启动TLS和管理总期限。
                return NI_NEED_MORE;
            }
            // 接收、认证和心跳由显式progress推进；管理发送队列为空时，
            // 转发每条业务密文不必再重复进入管理TLS的读写状态机。
            if (m_state->m_control->status() != NET_INTERFACE_TLS_READY || m_state->m_control->outputState() != NI_OK)
            {
                const int32_t progressed = m_state->m_control->progress(m_state->m_lastTime);
                if (progressed < 0)
                {
                    return m_state->fail(progressed);
                }
            }
            const uint8_t* output = nullptr;
            uint32_t size = 0;
            const int32_t result = m_state->m_control->peekOutput(output, size);
            if (result < 0)
            {
                return m_state->fail(result);
            }
            if (size != 0)
            {
                next.m_kind = NI_TUNNEL_CONTROL;
                next.m_data = output;
                next.m_size = (std::min)(size, NET_INTERFACE_TUNNEL_PAYLOAD_LIMIT);
            }
            else if (m_state->m_outputHead != nullptr)
            {
                NetInterfaceLinkWrite& front = *m_state->m_outputHead;
                next = front.m_record;
                next.m_data = front.m_bytes.data();
            }
            else
            {
                return NI_NEED_MORE;
            }
        }
        m_state->m_peeked = next;
        m_state->m_outputPeeked = true;
    }
    record = m_state->m_peeked;
    return NI_READY;
}

int32_t NetInterfaceLink::consumeOutput()
{
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    if (!m_state->m_outputPeeked)
    {
        return NI_INVALID_STATE;
    }
    const NetInterfaceTunnelRecord record = m_state->m_peeked;
    if (record.m_kind == NI_TUNNEL_HELLO)
    {
        m_state->m_sentHello = true;
    }
    else if (record.m_kind == NI_TUNNEL_CONTROL)
    {
        const int32_t result = m_state->m_control->consumeOutput(record.m_size);
        if (result < 0)
        {
            return m_state->fail(result);
        }
    }
    else
    {
        m_state->consumeQueuedRecord();
    }
    m_state->m_peeked = NetInterfaceTunnelRecord();
    m_state->m_outputPeeked = false;
    NetExecution::progress();
    return NI_OK;
}

int32_t NetInterfaceLink::sendRecord(const NetInterfaceTunnelRecord& record)
{
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    if (m_state->m_phase == NET_LINK_DRAINING)
    {
        return NI_INVALID_STATE;
    }
    if (m_state->status() != NI_READY)
    {
        return NI_INVALID_STATE;
    }
    uint32_t required = 0;
    if ((record.m_kind != NI_TUNNEL_BYTES && record.m_kind != NI_TUNNEL_CLOSE) ||
        NetInterfaceTunnel::encode(record, nullptr, 0, required) != NI_BUFFER_TOO_SMALL)
    {
        return NI_INVALID_ARGUMENT;
    }
    return m_state->queueRecord(record);
}

int32_t NetInterfaceLink::peekRecord(NetInterfaceTunnelRecord& record) const
{
    record = NetInterfaceTunnelRecord();
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    if (!m_state->m_recordReady)
    {
        return NI_NEED_MORE;
    }
    record = m_state->m_received;
    return NI_READY;
}

int32_t NetInterfaceLink::consumeRecord()
{
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    if (!m_state->m_recordReady)
    {
        return NI_INVALID_STATE;
    }
    m_state->m_recordReady = false;
    m_state->m_received = NetInterfaceTunnelRecord();
    m_state->m_receivedBytes.clear();
    NetExecution::progress();
    return NI_OK;
}

int32_t NetInterfaceLink::sendControl(const NetInterfaceFrame& frame)
{
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    if (m_state->m_phase == NET_LINK_DRAINING)
    {
        return NI_INVALID_STATE;
    }
    if (m_state->status() != NI_READY)
    {
        return NI_INVALID_STATE;
    }
    if (frame.m_productId != 0 || frame.m_payloadSize > 16384 ||
        (frame.m_kind != NI_REQUEST && frame.m_kind != NI_RESPONSE && frame.m_kind != NI_EVENT && frame.m_kind != NI_CANCEL) ||
        (frame.m_messageType >= NET_INTERFACE_LINK_CHALLENGE && frame.m_messageType <= NET_INTERFACE_LINK_HEARTBEAT))
    {
        return NI_INVALID_ARGUMENT;
    }
    return m_state->m_control->send(frame);
}

int32_t NetInterfaceLink::peekControl(NetInterfaceFrame& frame) const
{
    frame = NetInterfaceFrame();
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    if (m_state->status() != NI_READY)
    {
        return NI_NEED_MORE;
    }
    if (m_state->m_phase == NET_LINK_DRAINING && m_state->m_drainResult < 0)
    {
        // TLS终态不得越过已经由外层接纳、尚待上层消费的业务记录。
        return NI_NEED_MORE;
    }
    const int32_t result = m_state->m_control->peek(frame);
    if (result == NI_READY && frame.m_productId != 0)
    {
        frame = NetInterfaceFrame();
        return m_state->fail(NI_PROTOCOL_ERROR);
    }
    if (result == NI_READY && (frame.m_kind == NI_PING || frame.m_kind == NI_PONG ||
        frame.m_kind == NI_HELLO || frame.m_kind == NI_HELLO_REPLY))
    {
        frame = NetInterfaceFrame();
        return NI_NEED_MORE;
    }
    return result;
}

int32_t NetInterfaceLink::consumeControl()
{
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    NetInterfaceFrame frame = {};
    if (peekControl(frame) != NI_READY)
    {
        return NI_INVALID_STATE;
    }
    m_state->m_activity = m_state->m_lastTime;
    return m_state->m_control->consume();
}

int32_t NetInterfaceLink::peerInfo(NetInterfaceLinkPeer& peer) const
{
    peer = NetInterfaceLinkPeer();
    if (m_state->status() != NI_READY)
    {
        return m_state->status();
    }
    peer.m_role = m_state->m_peerRole;
    peer.m_name = m_state->m_peerName.c_str();
    peer.m_certificate = m_state->m_peerCertificate.empty() ? nullptr :
        reinterpret_cast<const uint8_t*>(m_state->m_peerCertificate.data());
    peer.m_certificateSize = static_cast<uint32_t>(m_state->m_peerCertificate.size());
    peer.m_claims = m_state->m_peerClaims.empty() ? nullptr : m_state->m_peerClaims.data();
    peer.m_claimsSize = static_cast<uint32_t>(m_state->m_peerClaims.size());
    return NI_READY;
}

int32_t NetInterfaceLink::serviceGrant(const uint8_t*& bytes, uint32_t& size) const
{
    bytes = nullptr;
    size = 0;
    if (m_state->status() != NI_READY)
    {
        return m_state->status();
    }
    if (m_state->m_options.m_role != NI_LINK_SERVICE)
    {
        return NI_INVALID_STATE;
    }
    bytes = m_state->m_grant.data();
    size = static_cast<uint32_t>(m_state->m_grant.size());
    return NI_READY;
}

int32_t NetInterfaceLink::finishInput()
{
    return m_state->fail(NI_TRUNCATED);
}

int32_t NetInterfaceLink::drainInput(uint64_t nowMs)
{
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    if (m_state->m_phase != NET_LINK_DRAINING)
    {
        if (m_state->status() != NI_READY)
        {
            return m_state->fail(NI_TRUNCATED);
        }
        m_state->m_phase = NET_LINK_DRAINING;
        m_state->m_drainStarted = nowMs;
    }
    return progress(nowMs);
}

int32_t NetInterfaceLink::status() const
{
    return m_state->status();
}

int32_t NetInterfaceLink::outputState() const
{
    if (m_state->status() < 0)
    {
        return m_state->status();
    }
    if (m_state->m_phase == NET_LINK_DRAINING)
    {
        return NI_INVALID_STATE;
    }
    if (!m_state->m_sentHello || m_state->m_outputHead != nullptr || m_state->m_outputPeeked)
    {
        return NI_READY;
    }
    return m_state->m_control ? m_state->m_control->outputState() : NI_INVALID_STATE;
}