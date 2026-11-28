#include "NetRelayTransportState.h"
#include "NetHostSocketState.h"
#include "NetPeerMacro.h"
#include <cstring>
#include <new>

NetRelayTransportWrite::NetRelayTransportWrite() :
m_size(0),
m_pool(nullptr),
m_storage(nullptr),
m_next(nullptr)
{

}

NetRelayTransportWrite::~NetRelayTransportWrite()
{
    NetInterface::wipe(m_bytes.data(), m_size);
}

NetRelayTransportPeer::NetRelayTransportPeer(uint64_t now, const NetRelayPoolConfig& pools) :
m_dataPool(pools.m_dataBytes),
m_controlPool(pools.m_controlBytes),
m_dataRecords(pools.m_dataRecords),
m_controlRecords(pools.m_controlRecords),
m_outputHead(nullptr),
m_outputTail(nullptr),
m_inputOffset(0),
m_started(now),
m_blockedSince(0),
m_phase(NET_RELAY_CONNECTING),
m_paused(false),
m_closed(false),
m_reason(NI_OK)
{

}

NetRelayTransportPeer::~NetRelayTransportPeer()
{
    NetInterface::wipe(m_input.empty() ? nullptr : m_input.data(), static_cast<uint32_t>(m_input.size()));
    while (m_outputHead != nullptr)
    {
        pop();
    }
}

int32_t NetRelayTransportPeer::status() const
{
    const int32_t states[] = {m_dataPool.status(), m_controlPool.status(), m_dataRecords.status(), m_controlRecords.status()};
    for (size_t index = 0; index < 4; ++index)
    {
        if (states[index] != NI_OK)
        {
            return states[index];
        }
    }
    return NI_OK;
}

int32_t NetRelayTransportPeer::enqueue(const NetInterfaceTunnelRecord& record)
{
    uint32_t required = 0;
    const int32_t checked = NetInterfaceTunnel::encode(record, nullptr, 0, required);
    if (checked != NI_BUFFER_TOO_SMALL)
    {
        return checked;
    }
    const bool control = record.m_kind != NI_TUNNEL_BYTES;
    NetObjectPool<NetRelayTransportWrite>& pool = control ? m_controlRecords : m_dataRecords;
    NetObjectReference<NetRelayTransportWrite> slot;
    int32_t result = pool.create(slot);
    if (result != NI_OK)
    {
        return result;
    }
    result = (control ? m_controlPool : m_dataPool).allocate(required, slot->m_bytes);
    if (result != NI_OK)
    {
        return result;
    }
    slot->m_size = required;
    result = NetInterfaceTunnel::encode(record, slot->m_bytes.data(), slot->m_bytes.capacity(), required);
    if (result != NI_OK)
    {
        return result;
    }
    slot->m_pool = &pool;
    slot->m_storage = slot.storage();
    NetRelayTransportWrite* write = slot.release();
    if (m_outputTail == nullptr)
    {
        m_outputHead = write;
        // 此运输本轮可能已经 poll；空队列首次接纳后，执行者下一轮即可尝试交付。
        NetExecution::progress();
    }
    else
    {
        m_outputTail->m_next = write;
    }
    m_outputTail = write;
    return NI_OK;
}

void NetRelayTransportPeer::pop()
{
    NetRelayTransportWrite* slot = m_outputHead;
    if (slot == nullptr)
    {
        return;
    }
    m_outputHead = slot->m_next;
    if (m_outputHead == nullptr)
    {
        m_outputTail = nullptr;
    }
    NetObjectPool<NetRelayTransportWrite>* pool = slot->m_pool;
    void* storage = slot->m_storage;
    pool->release(storage);
}

void NetRelayTransportPeer::clearInput()
{
    if (!m_input.empty())
    {
        std::memset(m_input.data(), 0, m_input.size());
        m_input.clear();
    }
    m_inputOffset = 0;
}

NetRelayTransportState::NetRelayTransportState(const NetHostPoolConfig& host, const NetRelayPoolConfig& pools) :
m_channel(nullptr, host),
m_listener(nullptr),
m_pools(new NetRelayPoolConfig(pools)),
m_phase(NET_RELAY_IDLE),
m_server(false),
m_polling(false)
{

}

uint64_t NetRelayTransportState::now()
{
    return NetInterface::monotonicMilliseconds();
}

void NetRelayTransportState::close(uint64_t id, int32_t reason)
{
    auto found = m_peers.find(id);
    if (found == m_peers.end() || found->second->m_phase == NET_RELAY_CLOSING)
    {
        return;
    }
    found->second->m_phase = NET_RELAY_CLOSING;
    found->second->m_reason = reason;
    m_channel.close(id, reason);
}

void NetRelayTransportState::onConnected(uint64_t id)
{
    if (m_phase != NET_RELAY_ACTIVE)
    {
        m_channel.close(id);
        return;
    }
    try
    {
        auto found = m_peers.find(id);
        if (found == m_peers.end())
        {
            std::unique_ptr<NetRelayTransportPeer> peer(new NetRelayTransportPeer(now(), *m_pools));
            if (peer->status() != NI_OK)
            {
                m_channel.close(id, peer->status());
                return;
            }
            found = m_peers.insert(std::make_pair(id, std::move(peer))).first;
        }
        NetRelayTransportPeer& peer = *found->second;
        if (peer.m_phase != NET_RELAY_CONNECTING)
        {
            return;
        }
        peer.m_phase = NET_RELAY_CONNECTED;
        peer.m_paused = true;
        const int32_t result = m_channel.pauseRead(id, true);
        if (result != NI_OK)
        {
            close(id, result);
        }
    }
    catch (...)
    {
        close(id, NI_NO_MEMORY);
        m_channel.close(id, NI_NO_MEMORY);
    }
}

void NetRelayTransportState::onData(uint64_t id, const uint8_t* data, uint32_t size)
{
    auto found = m_peers.find(id);
    if (m_phase != NET_RELAY_ACTIVE || found == m_peers.end() || found->second->m_phase == NET_RELAY_CLOSING)
    {
        return;
    }
    NetRelayTransportPeer& peer = *found->second;
    if (!peer.m_input.empty() || size > 16384)
    {
        close(id, NI_LIMIT_EXCEEDED);
        return;
    }
    if (data == nullptr || size == 0)
    {
        close(id, NI_INVALID_ARGUMENT);
        return;
    }
    try
    {
        // 先准备本代次的最大输入块，再复制，避免后续扩容遗留旧副本。
        peer.m_input.reserve(16384);
        peer.m_input.assign(data, data + size);
        peer.m_inputOffset = 0;
        peer.m_paused = true;
        const int32_t result = m_channel.pauseRead(id, true);
        if (result != NI_OK)
        {
            close(id, result);
        }
    }
    catch (...)
    {
        close(id, NI_NO_MEMORY);
    }
}

void NetRelayTransportState::onWritable(uint64_t peer)
{
    (void)peer;
}

void NetRelayTransportState::onClosed(uint64_t id, int32_t reason, int32_t detail)
{
    (void)detail;
    auto found = m_peers.find(id);
    if (found != m_peers.end())
    {
        NetRelayTransportPeer& peer = *found->second;
        peer.m_closed = true;
        if (peer.m_phase != NET_RELAY_CLOSING)
        {
            peer.m_reason = reason;
            if (peer.m_phase == NET_RELAY_CONNECTING)
            {
                peer.m_phase = NET_RELAY_CLOSING;
            }
        }
    }
}

void NetRelayTransportState::pump(uint64_t id, NetRelayTransportPeer& peer)
{
    if (peer.m_phase == NET_RELAY_CLOSING || m_phase != NET_RELAY_ACTIVE ||
        NetHostSocketState::pending(m_channel, id, NET_HOST_CLOSED))
    {
        return;
    }
    if (peer.m_phase == NET_RELAY_CONNECTING)
    {
        const uint64_t elapsed = now() - peer.m_started;
        NetExecution::deadline(m_pools->m_startupMs, elapsed);
        if (elapsed >= m_pools->m_startupMs)
        {
            close(id, NI_TIMEOUT);
        }
        return;
    }
    if (peer.m_phase == NET_RELAY_CONNECTED)
    {
        peer.m_phase = NET_RELAY_ESTABLISHED;
        m_listener->onRelayConnected(id);
    }
    uint32_t inputSteps = 0;
    for (; inputSteps < m_pools->m_stepBudget && peer.m_phase != NET_RELAY_CLOSING && m_phase == NET_RELAY_ACTIVE; ++inputSteps)
    {
        NetInterfaceTunnelRecord record = {};
        int32_t result = peer.m_decoder.peek(record);
        if (result == NI_READY)
        {
            const bool accepted = m_listener->onRelayRecord(id, record);
            if (peer.m_phase == NET_RELAY_CLOSING || m_phase != NET_RELAY_ACTIVE)
            {
                break;
            }
            if (!accepted)
            {
                const uint64_t current = now();
                if (peer.m_blockedSince == 0)
                {
                    peer.m_blockedSince = current;
                }
                const uint64_t elapsed = current - peer.m_blockedSince;
                NetExecution::deadline(m_pools->m_backpressureMs, elapsed);
                if (elapsed >= m_pools->m_backpressureMs)
                {
                    close(id, NI_TIMEOUT);
                }
                if (!peer.m_paused && !peer.m_closed)
                {
                    const int32_t paused = m_channel.pauseRead(id, true);
                    peer.m_paused = true;
                    if (paused != NI_OK)
                    {
                        close(id, paused);
                    }
                }
                break;
            }
            peer.m_blockedSince = 0;
            peer.m_decoder.consume();
        }
        else if (result < 0)
        {
            close(id, result);
            break;
        }
        else if (peer.m_inputOffset < peer.m_input.size())
        {
            uint32_t consumed = 0;
            result = peer.m_decoder.feed(peer.m_input.data() + peer.m_inputOffset,
                static_cast<uint32_t>(peer.m_input.size()) - peer.m_inputOffset, consumed);
            peer.m_inputOffset += consumed;
            if (result < 0)
            {
                close(id, result);
                break;
            }
        }
        else
        {
            peer.clearInput();
            if (peer.m_closed)
            {
                const int32_t finished = peer.m_decoder.finish();
                peer.m_phase = NET_RELAY_CLOSING;
                if (finished < 0 && peer.m_reason == NI_OK)
                {
                    peer.m_reason = finished;
                }
            }
            else if (peer.m_paused)
            {
                const int32_t resumed = m_channel.pauseRead(id, false);
                peer.m_paused = false;
                if (resumed != NI_OK)
                {
                    close(id, resumed);
                }
            }
            break;
        }
    }
    if (inputSteps == m_pools->m_stepBudget)
    {
        // 已消耗本轮量子，下一轮还须检查解码器、未读字节或恢复宿主读取。
        NetExecution::progress();
    }
    for (uint32_t step = 0; step < m_pools->m_stepBudget && peer.m_outputHead != nullptr &&
        !peer.m_closed && peer.m_phase != NET_RELAY_CLOSING && m_phase == NET_RELAY_ACTIVE;)
    {
        NetRelayTransportWrite& write = *peer.m_outputHead;
        uint32_t count = 1;
        uint32_t size = write.m_size;
        const NetRelayTransportWrite* next = write.m_next;
        while (next != nullptr && count < m_pools->m_stepBudget - step)
        {
            if (next->m_size > 32768 - size)
            {
                break;
            }
            size += next->m_size;
            next = next->m_next;
            ++count;
        }
        if (count > 1 && !m_batch)
        {
            m_batch.reset(new (std::nothrow) uint8_t[32768]);
        }
        const uint8_t* data = write.m_bytes.data();
        if (count > 1 && m_batch)
        {
            uint32_t offset = 0;
            const NetRelayTransportWrite* part = &write;
            for (uint32_t index = 0; index < count; ++index)
            {
                std::memcpy(m_batch.get() + offset, part->m_bytes.data(), part->m_size);
                offset += part->m_size;
                part = part->m_next;
            }
            data = m_batch.get();
        }
        else
        {
            count = 1;
            size = write.m_size;
        }
        int32_t result = m_channel.write(id, data, size);
        if (count > 1)
        {
            // 宿主只借用到返回；临时副本包含密文，队列原件只在完整接纳后释放。
            std::memset(m_batch.get(), 0, size);
            if (NetHostSocket::isBackpressure(result))
            {
                // 剩余容量可能容得下队首；不能因合并包过大使本可前进的连接停住。
                count = 1;
                result = m_channel.write(id, write.m_bytes.data(), write.m_size);
            }
        }
        if (NetHostSocket::isBackpressure(result) || (result < 0 && NetHostSocketState::pending(m_channel, id, NET_HOST_CLOSED)))
        {
            break;
        }
        if (result < 0)
        {
            close(id, result);
            break;
        }
        for (uint32_t index = 0; index < count; ++index)
        {
            peer.pop();
        }
        step += count;
        if (step == m_pools->m_stepBudget && peer.m_outputHead != nullptr)
        {
            NetExecution::progress();
        }
    }
}