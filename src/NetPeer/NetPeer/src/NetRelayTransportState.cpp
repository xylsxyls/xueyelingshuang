#include "NetRelayTransportState.h"
#include "NetPeerMacro.h"
#include <chrono>
#include <cstring>

NetRelayTransportWrite::NetRelayTransportWrite() :
m_size(0),
m_capacity(0),
m_control(false)
{

}

NetRelayTransportWrite::~NetRelayTransportWrite()
{
    NetInterface::wipe(m_bytes.get(), m_capacity);
}

NetRelayTransportPeer::NetRelayTransportPeer(uint64_t now) :
m_recycledCapacity(0),
m_outputHead(0),
m_inputOffset(0),
m_dataBytes(0),
m_controlBytes(0),
m_dataCount(0),
m_controlCount(0),
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
    NetInterface::wipe(m_recycled.get(), m_recycledCapacity);
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
    const uint32_t limit = (NET_INTERFACE_TUNNEL_PAYLOAD_LIMIT + NET_INTERFACE_TUNNEL_HEADER_SIZE) * (control ? 2 : 4);
    uint32_t& bytes = control ? m_controlBytes : m_dataBytes;
    uint32_t& count = control ? m_controlCount : m_dataCount;
    if (required > limit - bytes || count >= 32)
    {
        return NI_LIMIT_EXCEEDED;
    }
    NetRelayTransportWrite& slot = m_output[(m_outputHead + m_dataCount + m_controlCount) % 64];
    try
    {
        if (m_recycledCapacity >= required)
        {
            slot.m_bytes.swap(m_recycled);
            slot.m_capacity = m_recycledCapacity;
            m_recycledCapacity = 0;
        }
        else
        {
            slot.m_bytes.reset(new uint8_t[required]);
            slot.m_capacity = required;
        }
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
    const int32_t encoded = NetInterfaceTunnel::encode(record, slot.m_bytes.get(), slot.m_capacity, required);
    if (encoded != NI_OK)
    {
        // 未提交槽位和额度；encode拒绝时不改输出，缓冲可直接释放。
        slot.m_bytes.reset();
        slot.m_capacity = 0;
        return encoded;
    }
    slot.m_size = required;
    slot.m_control = control;
    bytes += required;
    ++count;
    return NI_OK;
}

void NetRelayTransportPeer::pop()
{
    if (m_dataCount + m_controlCount == 0)
    {
        return;
    }
    NetRelayTransportWrite& slot = m_output[m_outputHead];
    uint32_t& bytes = slot.m_control ? m_controlBytes : m_dataBytes;
    uint32_t& count = slot.m_control ? m_controlCount : m_dataCount;
    bytes -= slot.m_size;
    --count;
    // 本层只持有外层头和密文，不含密钥或业务明文；复用时整块清零，释放仍做安全擦除。
    std::memset(slot.m_bytes.get(), 0, slot.m_size);
    if (slot.m_capacity > m_recycledCapacity)
    {
        slot.m_bytes.swap(m_recycled);
        m_recycledCapacity = slot.m_capacity;
    }
    slot.m_bytes.reset();
    slot.m_size = 0;
    slot.m_capacity = 0;
    slot.m_control = false;
    m_outputHead = (m_outputHead + 1) % 64;
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

NetRelayTransportState::NetRelayTransportState() :
m_listener(nullptr),
m_phase(NET_RELAY_IDLE),
m_server(false),
m_polling(false)
{

}

uint64_t NetRelayTransportState::now()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
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
            std::unique_ptr<NetRelayTransportPeer> peer(new NetRelayTransportPeer(now()));
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
    if (peer.m_phase == NET_RELAY_CLOSING || m_phase != NET_RELAY_ACTIVE)
    {
        return;
    }
    if (peer.m_phase == NET_RELAY_CONNECTING)
    {
        if (now() - peer.m_started >= 5000)
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
    for (uint32_t step = 0; step < 64 && peer.m_phase != NET_RELAY_CLOSING && m_phase == NET_RELAY_ACTIVE; ++step)
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
                if (peer.m_blockedSince == 0)
                {
                    peer.m_blockedSince = now();
                }
                else if (now() - peer.m_blockedSince >= 5000)
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
    for (uint32_t step = 0; step < 64 && peer.m_dataCount + peer.m_controlCount != 0 &&
        !peer.m_closed && peer.m_phase != NET_RELAY_CLOSING && m_phase == NET_RELAY_ACTIVE; ++step)
    {
        NetRelayTransportWrite& write = peer.m_output[peer.m_outputHead];
        const int32_t result = m_channel.write(id, write.m_bytes.get(), write.m_size);
        if (NetHostSocket::isBackpressure(result))
        {
            break;
        }
        if (result < 0)
        {
            close(id, result);
            break;
        }
        peer.pop();
    }
}