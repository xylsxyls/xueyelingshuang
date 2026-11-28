#include "NetRelayTransport.h"
#include "NetPeerMacro.h"
#include "NetRelayTransportState.h"
#include "NetHostSocketState.h"

NetRelayTransportListener::~NetRelayTransportListener()
{

}

NetRelayTransport::NetRelayTransport() : NetRelayTransport(NetHostPoolConfig(), NetRelayPoolConfig())
{
}

NetRelayTransport::NetRelayTransport(const NetHostPoolConfig& host, const NetRelayPoolConfig& pools) :
m_state(new NetRelayTransportState(host, pools))
{

}

NetRelayTransport::~NetRelayTransport()
{
    shutdown();
    delete m_state;
}

int32_t NetRelayTransport::configurePools(const NetRelayPoolConfig& pools)
{
    if (m_state->m_phase != NET_RELAY_IDLE)
    {
        return NI_INVALID_STATE;
    }
    if (!pools.valid()) { return NI_INVALID_ARGUMENT; }
    try
    {
        std::unique_ptr<NetRelayPoolConfig> copy(new NetRelayPoolConfig(pools));
        NetRelayTransportPeer validation(0, *copy);
        if (validation.status() != NI_OK)
        {
            return validation.status();
        }
        m_state->m_pools.swap(copy);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetRelayTransport::listen(const char* address, uint32_t port, NetRelayTransportListener* listener, uint32_t maxPeers)
{
    if (!NetHostSocketState::validListen(address, port, maxPeers) || m_state->m_phase != NET_RELAY_IDLE ||
        listener == nullptr || !m_state->m_pools->valid())
    {
        return NI_INVALID_ARGUMENT;
    }
    m_state->m_listener = listener;
    m_state->m_phase = NET_RELAY_STARTING;
    const int32_t result = m_state->m_channel.listen(address, port, m_state, maxPeers);
    m_state->m_server = result == NI_OK;
    m_state->m_phase = result == NI_OK ? NET_RELAY_ACTIVE : NET_RELAY_STOPPING;
    if (result != NI_OK)
    {
        shutdown();
    }
    return result;
}

int32_t NetRelayTransport::connect(const char* address, uint32_t port, NetRelayTransportListener* listener, uint64_t& peer)
{
    peer = 0;
    if (!NetHostSocketState::validConnect(address, port) || m_state->m_phase != NET_RELAY_IDLE ||
        listener == nullptr || !m_state->m_pools->valid())
    {
        return NI_INVALID_ARGUMENT;
    }
    m_state->m_listener = listener;
    m_state->m_phase = NET_RELAY_STARTING;
    int32_t result = m_state->m_channel.connect(address, port, m_state, peer);
    if (result == NI_OK)
    {
        try
        {
            std::unique_ptr<NetRelayTransportPeer> entry(new NetRelayTransportPeer(NetRelayTransportState::now(), *m_state->m_pools));
            result = entry->status();
            if (result == NI_OK)
            {
                m_state->m_peers.insert(std::make_pair(peer, std::move(entry)));
            }
            else
            {
                m_state->m_channel.close(peer, result);
                peer = 0;
            }
        }
        catch (...)
        {
            m_state->m_channel.close(peer, NI_NO_MEMORY);
            peer = 0;
            result = NI_NO_MEMORY;
        }
    }
    m_state->m_phase = result == NI_OK ? NET_RELAY_ACTIVE : NET_RELAY_STOPPING;
    if (result != NI_OK)
    {
        shutdown();
    }
    return result;
}

bool NetRelayTransport::poll()
{
    if (m_state->m_polling)
    {
        return m_state->m_phase == NET_RELAY_ACTIVE;
    }
    if (m_state->m_phase != NET_RELAY_ACTIVE)
    {
        return false;
    }
    m_state->m_polling = true;
    m_state->m_channel.poll();
    if (m_state->m_channel.status() < 0)
    {
        // 宿主已停止监听，仍须先结算EOF前已接纳的输入和终态。
        m_state->m_server = false;
    }
    for (auto entry = m_state->m_peers.begin(); entry != m_state->m_peers.end();)
    {
        const uint64_t id = entry->first;
        NetRelayTransportPeer& peer = *entry->second;
        try
        {
            m_state->pump(id, peer);
        }
        catch (...)
        {
            m_state->close(id, NI_NO_MEMORY);
        }
        if (peer.m_closed && peer.m_phase == NET_RELAY_CLOSING)
        {
            const int32_t reason = peer.m_reason;
            entry = m_state->m_peers.erase(entry);
            try
            {
                m_state->m_listener->onRelayClosed(id, reason);
            }
            catch (...)
            {
                // 先回收再通知；监听者异常不得重复关闭或带出事件循环。
            }
        }
        else
        {
            ++entry;
        }
    }
    m_state->m_polling = false;
    if (m_state->m_phase == NET_RELAY_STOPPING)
    {
        shutdown();
    }
    return m_state->m_phase == NET_RELAY_ACTIVE && (m_state->m_server || !m_state->m_peers.empty());
}

int32_t NetRelayTransport::send(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    auto found = m_state->m_peers.find(peer);
    if (m_state->m_phase != NET_RELAY_ACTIVE || found == m_state->m_peers.end() ||
        found->second->m_phase == NET_RELAY_CONNECTING || found->second->m_phase == NET_RELAY_CLOSING || found->second->m_closed ||
        NetHostSocketState::pending(m_state->m_channel, peer, NET_HOST_CLOSED))
    {
        return NI_INVALID_STATE;
    }
    return found->second->enqueue(record);
}

void NetRelayTransport::close(uint64_t peer, int32_t reason)
{
    m_state->close(peer, reason);
}

void NetRelayTransport::shutdown()
{
    if (m_state->m_phase == NET_RELAY_STOPPED)
    {
        return;
    }
    m_state->m_phase = NET_RELAY_STOPPING;
    if (m_state->m_polling)
    {
        return;
    }
    m_state->m_polling = true;
    for (auto& entry : m_state->m_peers)
    {
        m_state->close(entry.first, NI_OK);
    }
    m_state->m_channel.shutdown();
    while (!m_state->m_peers.empty())
    {
        auto entry = m_state->m_peers.begin();
        const uint64_t id = entry->first;
        const int32_t reason = entry->second->m_reason;
        m_state->m_peers.erase(entry);
        try
        {
            m_state->m_listener->onRelayClosed(id, reason);
        }
        catch (...)
        {
            // 终态已结算，继续释放其余连接。
        }
    }
    m_state->m_server = false;
    m_state->m_phase = NET_RELAY_STOPPED;
    m_state->m_polling = false;
}

uint32_t NetRelayTransport::localPort() const
{
    return m_state->m_channel.localPort();
}

int32_t NetRelayTransport::outputState(uint64_t peer) const
{
    const auto found = m_state->m_peers.find(peer);
    if (m_state->m_phase != NET_RELAY_ACTIVE || found == m_state->m_peers.end() ||
        found->second->m_closed || found->second->m_phase == NET_RELAY_CLOSING || NetHostSocketState::pending(m_state->m_channel, peer, NET_HOST_CLOSED))
    {
        return NI_INVALID_STATE;
    }
    return found->second->m_outputHead == nullptr ? NI_OK : NI_READY;
}