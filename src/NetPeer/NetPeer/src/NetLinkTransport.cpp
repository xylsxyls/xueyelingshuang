#include "NetLinkTransport.h"
#include "NetPeerMacro.h"
#include "NetLinkTransportState.h"
#include "NetTransport.h"
#include <chrono>

NetLinkTransport::NetLinkTransport() : m_state(new NetLinkTransportState())
{

}

NetLinkTransport::~NetLinkTransport()
{
    shutdown();
    delete m_state;
}

int32_t NetLinkTransport::configurePeerPool(const LockFreeHashMapPoolConfig& config)
{
    if (m_state->m_started || m_state->m_stopping) { return NI_INVALID_STATE; }
    try
    {
        std::unique_ptr<NetObjectPool<NetLinkTransportPeer>> pool(new NetObjectPool<NetLinkTransportPeer>(config));
        if (pool->status() != NI_OK) { return pool->status(); }
        m_state->m_peerPool.swap(pool);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetLinkTransport::listen(const char* address, uint32_t port, const NetInterfaceLinkOptions& options,
    NetLinkTransportListener* listener, uint32_t maxPeers)
{
    // 该适配器是TCP公共/节点入口，服务首次注册只允许独立本机IPC适配。
    if (options.m_tls.server != 1 || (options.m_peerRoles & (1U << NI_LINK_SERVICE)) != 0) { return NI_INVALID_ARGUMENT; }
    const int32_t configured = m_state->configure(options, listener);
    return configured == NI_OK ? m_state->m_raw.listen(address, port, m_state, maxPeers) : configured;
}

int32_t NetLinkTransport::connect(const char* address, uint32_t port, const NetInterfaceLinkOptions& options,
    NetLinkTransportListener* listener, uint64_t& peer)
{
    peer = 0;
    if (options.m_tls.server != 0) { return NI_INVALID_ARGUMENT; }
    const int32_t configured = m_state->configure(options, listener);
    if (configured != NI_OK) { return configured; }
    const int32_t connected = m_state->m_raw.connect(address, port, m_state, peer);
    if (connected != NI_OK) { return connected; }
    try
    {
        NetObjectReference<NetLinkTransportPeer> entry;
        const int32_t allocated = m_state->m_peerPool->create(entry, m_state->m_options);
        if (allocated != NI_OK) { m_state->m_raw.close(peer, allocated); peer = 0; return allocated; }
        const int32_t status = entry->m_link.status();
        m_state->m_peers.insert(std::make_pair(peer, std::move(entry)));
        if (status < 0) { m_state->close(peer, status); peer = 0; return status; }
        return NI_OK;
    }
    catch (...)
    {
        m_state->m_raw.close(peer, NI_NO_MEMORY);
        peer = 0;
        return NI_NO_MEMORY;
    }
}

bool NetLinkTransport::poll()
{
    if (m_state->m_polling) { return !m_state->m_stopping; }
    m_state->m_polling = true;
    const bool active = m_state->m_raw.poll();
    for (auto& entry : m_state->m_peers)
    {
        try { m_state->pump(entry.first, *entry.second); }
        catch (...) { m_state->close(entry.first, NI_NO_MEMORY); }
    }
    m_state->collectClosed();
    m_state->m_polling = false;
    if (m_state->m_stopping) { shutdown(); }
    return (active || !m_state->m_peers.empty()) && !m_state->m_stopping;
}

int32_t NetLinkTransport::sendControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    auto found = m_state->m_peers.find(peer);
    return m_state->m_stopping || found == m_state->m_peers.end() || (found->second->m_closing || found->second->m_closed) ? NI_INVALID_STATE :
        found->second->m_link.sendControl(frame);
}

int32_t NetLinkTransport::sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    auto found = m_state->m_peers.find(peer);
    return m_state->m_stopping || found == m_state->m_peers.end() || (found->second->m_closing || found->second->m_closed) ? NI_INVALID_STATE :
        found->second->m_link.sendRecord(record);
}

void NetLinkTransport::close(uint64_t peer, int32_t reason)
{
    m_state->close(peer, reason);
}

void NetLinkTransport::shutdown()
{
    m_state->m_stopping = true;
    if (m_state->m_polling) { return; }
    m_state->m_polling = true;
    for (auto& entry : m_state->m_peers) { m_state->close(entry.first, 0); }
    m_state->m_raw.shutdown();
    m_state->collectClosed();
    m_state->m_polling = false;
}

uint32_t NetLinkTransport::localPort() const
{
    return m_state->m_raw.localPort();
}

int32_t NetLinkTransport::outputState(uint64_t peer) const
{
    const auto found = m_state->m_peers.find(peer);
    if (found == m_state->m_peers.end() || found->second->m_closing || found->second->m_closed) { return NI_INVALID_STATE; }
    const int32_t local = found->second->m_link.outputState();
    return local == NI_OK ? m_state->m_raw.outputState(peer) : local;
}