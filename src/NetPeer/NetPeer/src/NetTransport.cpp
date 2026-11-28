#include "NetPeerMacro.h"
#include "NetTransport.h"
#include "NetTransportState.h"
#include "NetHostSocketState.h"

int32_t NetTransport::configurePeerPool(const LockFreeHashMapPoolConfig& config)
{
    if (m_state->m_started || m_state->m_stopping) { return NI_INVALID_STATE; }
    try
    {
        std::unique_ptr<NetObjectPool<NetTransportPeer>> pool(new NetObjectPool<NetTransportPeer>(config));
        if (pool->status() != NI_OK) { return pool->status(); }
        m_state->m_peerPool.swap(pool);
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetTransport::randomBytes(uint8_t* output, uint32_t size)
{
    return NetHostSocket::randomBytes(output, size);
}

NetTransport::NetTransport() : NetTransport(NetDirectTransportConfig())
{
}

NetTransport::NetTransport(const NetDirectTransportConfig& config) : m_state(new NetTransportState(config))
{

}

NetTransport::~NetTransport()
{
    delete m_state;
}

int32_t NetTransport::configureConnectionPools(const NetConnectionPoolConfig& config)
{
    if (m_state->m_started || m_state->m_stopping || m_state->m_polling)
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

int32_t NetTransport::listen(const char* address, uint32_t port,
    const NetInterfaceTlsOptions& options, NetTransportListener* listener, uint32_t maxPeers)
{
    if (!NetHostSocketState::validListen(address, port, maxPeers))
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
    if (!NetHostSocketState::validConnect(address, port))
    {
        return NI_INVALID_ARGUMENT;
    }
    const int32_t configured = m_state->configure(options, listener, false);
    if (configured != 0)
    {
        return configured;
    }
    try
    {
        // 在提交底层连接之前准备拥有者，后续容器分配失败必须撤销底层请求。
        NetObjectReference<NetTransportPeer> peer;
        const int32_t allocated = m_state->m_peerPool->create(peer);
        if (allocated != NI_OK)
        {
            // 尚未调用宿主；归还这次启动机会，让调用方能更正池配置。
            m_state->m_started = false;
            m_state->m_listener = nullptr;
            return allocated;
        }
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
    NetInterfaceConnection* connection = m_state->writableConnection(id);
    return connection ? connection->send(frame) : NI_INVALID_STATE;
}

int32_t NetTransport::configureCompression(uint64_t peer, const uint32_t* types, uint32_t count, bool negotiated)
{
    NetInterfaceConnection* connection = m_state->writableConnection(peer);
    return connection ? connection->configureCompression(types, count, negotiated) : NI_INVALID_STATE;
}

int32_t NetTransport::configureCompact(uint64_t peer, bool enabled)
{
    NetInterfaceConnection* connection = m_state->writableConnection(peer);
    if (!connection) { return NI_INVALID_STATE; }
    connection->configureCompact(enabled);
    return NI_OK;
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
    NetInterfaceConnection* connection = m_state->writableConnection(peer);
    return connection ? connection->outputState() : NI_INVALID_STATE;
}