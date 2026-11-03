#include "NetPeerMacro.h"
#include "NetTransport.h"
#include "NetTransportState.h"
#include "NetTransportEntropy.h"

int32_t NetTransport::randomBytes(uint8_t* output, uint32_t size)
{
    return NetTransportEntropy(nullptr, output, size) == 0 ? NI_OK : NI_INVALID_STATE;
}

NetTransport::NetTransport() : m_state(new NetTransportState)
{

}

NetTransport::~NetTransport()
{
    delete m_state;
}

int32_t NetTransport::listen(const char* address, uint32_t port,
    const NetInterfaceTlsOptions& options, NetTransportListener* listener, uint32_t maxPeers)
{
    if (maxPeers == 0 || maxPeers > 256)
    {
        return NI_INVALID_ARGUMENT;
    }
    const int32_t configured = m_state->configure(options, listener, true);
    return configured != 0 ? configured : m_state->m_channel.listen(address, port, m_state, maxPeers);
}

int32_t NetTransport::connect(const char* address, uint32_t port,
    const NetInterfaceTlsOptions& options, NetTransportListener* listener, uint64_t& id)
{
    id = 0;
    const int32_t configured = m_state->configure(options, listener, false);
    if (configured != 0)
    {
        return configured;
    }
    try
    {
        // 在提交底层连接之前准备拥有者，后续容器分配失败必须撤销底层请求。
        std::unique_ptr<NetTransportPeer> peer(new NetTransportPeer);
        peer->m_started = NetTransportState::now();
        const int32_t result = m_state->m_channel.connect(address, port, m_state, id);
        if (result < 0)
        {
            return result;
        }
        m_state->m_peers.insert(std::make_pair(id, std::move(peer)));
        return 0;
    }
    catch (...)
    {
        m_state->m_channel.close(id, NI_NO_MEMORY);
        id = 0;
        return NI_NO_MEMORY;
    }
}

bool NetTransport::poll()
{
    return m_state->poll();
}

int32_t NetTransport::send(uint64_t id, const NetInterfaceFrame& frame)
{
    auto entry = m_state->m_peers.find(id);
    if (m_state->m_stopping || entry == m_state->m_peers.end() ||
        entry->second->m_closing || !entry->second->m_connection)
    {
        return NI_INVALID_STATE;
    }
    return entry->second->m_connection->send(frame);
}

int32_t NetTransport::configureCompression(uint64_t peer, const uint32_t* types, uint32_t count, bool negotiated)
{
    const auto found = m_state->m_peers.find(peer);
    if (found == m_state->m_peers.end() || found->second->m_closing || !found->second->m_connection) { return NI_INVALID_STATE; }
    return found->second->m_connection->configureCompression(types, count, negotiated);
}

void NetTransport::close(uint64_t id, int32_t reason)
{
    m_state->close(id, reason);
}

void NetTransport::shutdown()
{
    m_state->shutdown();
}

uint32_t NetTransport::localPort() const
{
    return m_state->m_channel.localPort();
}

uint32_t NetTransport::peerCount() const
{
    return static_cast<uint32_t>(m_state->m_peers.size());
}

int32_t NetTransport::outputState(uint64_t peer) const
{
    const auto found = m_state->m_peers.find(peer);
    if (found == m_state->m_peers.end() || found->second->m_closing || !found->second->m_connection) { return NI_INVALID_STATE; }
    return found->second->m_connection->outputState();
}