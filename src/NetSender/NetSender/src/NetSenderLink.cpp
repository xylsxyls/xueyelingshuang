#include "NetSenderLink.h"
#include "NetSenderMacro.h"
#include "NetSenderLinkState.h"
#include "CSystem/CSystemAPI.h"
#include <chrono>

NetSenderLink::NetSenderLink(NetSenderLocalInbox& inbox) : m_state(new NetSenderLinkState(inbox))
{

}

NetSenderLink::~NetSenderLink()
{
    shutdown();
    delete m_state;
}

int32_t NetSenderLink::configurePeerPool(const LockFreeHashMapPoolConfig& config)
{
    if (m_state->m_started || m_state->m_stopping) { return NI_INVALID_STATE; }
    try
    {
        std::unique_ptr<NetObjectPool<NetSenderLinkPeer>> pool(new NetObjectPool<NetSenderLinkPeer>(config));
        if (pool->status() != NI_OK) { return pool->status(); }
        m_state->m_peerPool.swap(pool);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetSenderLink::listen(const NetInterfaceLinkOptions& options, NetLinkTransportListener* listener, uint32_t maximum)
{
    if (options.m_tls.server != 1) { return NI_INVALID_ARGUMENT; }
    const int32_t configured = m_state->configure(options, listener);
    return configured == NI_OK ? m_state->m_raw.listenRaw(m_state->m_options.m_tls.entropy,
        m_state->m_options.m_tls.entropyContext, m_state, maximum) : configured;
}

int32_t NetSenderLink::connect(int32_t pid, const NetInterfaceLinkOptions& options, NetLinkTransportListener* listener, uint64_t& peer)
{
    peer = 0;
    if (options.m_tls.server != 0) { return NI_INVALID_ARGUMENT; }
    const int32_t configured = m_state->configure(options, listener);
    if (configured != NI_OK) { return configured; }
    const int32_t connected = m_state->m_raw.connectRaw(pid, m_state->m_options.m_tls.entropy,
        m_state->m_options.m_tls.entropyContext, m_state, peer);
    if (connected != NI_OK) { return connected; }
    try
    {
        NetObjectReference<NetSenderLinkPeer> entry;
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

bool NetSenderLink::poll()
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

int32_t NetSenderLink::sendControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    auto found = m_state->m_peers.find(peer);
    return m_state->m_stopping || found == m_state->m_peers.end() || (found->second->m_closing || found->second->m_closed) ? NI_INVALID_STATE :
        found->second->m_link.sendControl(frame);
}

int32_t NetSenderLink::sendRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    auto found = m_state->m_peers.find(peer);
    return m_state->m_stopping || found == m_state->m_peers.end() || (found->second->m_closing || found->second->m_closed) ? NI_INVALID_STATE :
        found->second->m_link.sendRecord(record);
}

int32_t NetSenderLink::serviceGrant(uint64_t peer, std::string& certificate) const
{
    certificate.clear();
    auto found = m_state->m_peers.find(peer);
    if (m_state->m_stopping || found == m_state->m_peers.end() || found->second->m_closing) { return NI_INVALID_STATE; }
    const uint8_t* bytes = nullptr;
    uint32_t size = 0;
    const int32_t ready = found->second->m_link.serviceGrant(bytes, size);
    if (ready == NI_READY)
    {
        try
        {
            certificate.assign(reinterpret_cast<const char*>(bytes), size);
        }
        catch (...)
        {
            certificate.clear();
            return NI_NO_MEMORY;
        }
    }
    return ready;
}

void NetSenderLink::close(uint64_t peer, int32_t reason)
{
    m_state->close(peer, reason);
}

void NetSenderLink::shutdown()
{
    m_state->m_stopping = true;
    if (m_state->m_polling) { return; }
    m_state->m_polling = true;
    for (auto& entry : m_state->m_peers) { m_state->close(entry.first, 0); }
    m_state->m_raw.shutdown();
    m_state->collectClosed();
    m_state->m_polling = false;
}

int32_t NetSenderLink::outputState(uint64_t peer) const
{
    const auto found = m_state->m_peers.find(peer);
    if (found == m_state->m_peers.end() || found->second->m_closing || found->second->m_closed) { return NI_INVALID_STATE; }
    const int32_t local = found->second->m_link.outputState();
    return local == NI_OK ? m_state->m_raw.outputState(peer) : local;
}