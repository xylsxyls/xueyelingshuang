#include "NetClientOpaqueGateway.h"
#include "NetClientOpaqueGatewayPeer.h"

/** 本机公开代次使用系统强随机
@param [in] context 忽略
@param [out] output 输出字节
@param [in] size 字节数
@return 0成功，否则失败
*/
static int32_t OpaqueGatewayEntropy(void* context, uint8_t* output, uint32_t size)
{
    (void)context;
    return NetTransport::randomBytes(output, size) == NI_OK ? 0 : -1;
}

NetClientOpaqueGatewayPeer::NetClientOpaqueGatewayPeer(NetClientOpaqueGateway& owner, uint64_t local) :
m_owner(&owner),
m_local(local),
m_remote(0),
m_address(),
m_connected(false),
m_closed(false)
{
    m_address.m_structSize = sizeof(m_address);
    m_address.m_version = 1;
}

NetClientOpaqueGatewayPeer::~NetClientOpaqueGatewayPeer()
{
    m_transport.shutdown();
}

void NetClientOpaqueGatewayPeer::onRelayConnected(uint64_t peer)
{
    m_remote = peer;
    m_connected = true;
}

bool NetClientOpaqueGatewayPeer::onRelayRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    if (m_closed || peer != m_remote) { return true; }
    const int32_t sent = m_owner->m_local.sendRecord(m_local, record);
    if (sent == NI_LIMIT_EXCEEDED) { return false; }
    if (sent != NI_OK)
    {
        m_closed = true;
        m_transport.close(peer, sent);
        m_owner->m_local.close(m_local, sent);
    }
    return true;
}

void NetClientOpaqueGatewayPeer::onRelayClosed(uint64_t peer, int32_t reason)
{
    if (peer != m_remote && m_remote != 0) { return; }
    m_connected = false;
    m_owner->remoteClosed(m_local, reason);
    m_closed = true;
}

NetClientOpaqueGateway::NetClientOpaqueGateway() : m_local(m_inbox), m_receiving(false),
    m_started(false), m_stopping(false)
{

}

NetClientOpaqueGateway::~NetClientOpaqueGateway()
{
    shutdown();
}

int32_t NetClientOpaqueGateway::init(const std::vector<NetClientGatewayAddress>& addresses, const std::string& cachePath)
{
    if (m_started || m_stopping || addresses.empty() || addresses.size() > 64) { return NI_INVALID_ARGUMENT; }
    for (const NetClientGatewayAddress& address : addresses)
    {
        if (address.m_address.empty() || address.m_port == 0 || address.m_port > 65535) { return NI_INVALID_ARGUMENT; }
    }
    try
    {
        if (ProcessWork::instance().receiveBufferCount() != 0)
        {
            return NI_INVALID_STATE;
        }
        const int32_t discovery = m_discovery.start(addresses, cachePath);
        if (discovery != NI_OK) { return discovery; }
        ProcessWork::instance().addProcessReceiveCallback(&m_inbox);
        if (!ProcessWork::instance().initReceive(16452, 64))
        {
            ProcessWork::instance().uninitReceive();
            m_local.shutdown();
            return NI_INVALID_STATE;
        }
        m_receiving = true;
        // 透明路由与TLS路径遵守同一个接收容量及启动次序。
        const int32_t listening = m_local.listenRaw(OpaqueGatewayEntropy, nullptr, this, 32);
        if (listening != NI_OK)
        {
            shutdown(); return listening;
        }
        m_started = true;
        return NI_OK;
    }
    catch (...) { shutdown(); return NI_NO_MEMORY; }
}

void NetClientOpaqueGateway::onRawLocalReady(uint64_t local, int32_t pid)
{
    (void)pid;
    if (m_stopping || m_peers.size() >= 32) { m_local.close(local, NI_LIMIT_EXCEEDED); return; }
    NetClientDiscoveryPoint address = {};
    address.m_structSize = sizeof(address);
    address.m_version = 1;
    if (!m_discovery.choose(address)) { m_local.close(local, NI_NOT_FOUND); return; }
    std::unique_ptr<NetClientOpaqueGatewayPeer> peer(new NetClientOpaqueGatewayPeer(*this, local));
    peer->m_address = address;
    uint64_t remote = 0;
    const int32_t connected = peer->m_transport.connect(address.m_address, address.m_port, peer.get(), remote);
    if (connected != NI_OK)
    {
        m_discovery.failed(address);
        m_local.close(local, connected);
        return;
    }
    peer->m_remote = remote;
    m_peers.insert(std::make_pair(local, std::move(peer)));
}

bool NetClientOpaqueGateway::onRawLocalRecord(uint64_t local, const NetInterfaceTunnelRecord& record)
{
    auto found = m_peers.find(local);
    if (m_stopping || found == m_peers.end() || found->second->m_closed) { m_local.close(local, NI_INVALID_STATE); return true; }
    if (!found->second->m_connected) { return false; }
    const int32_t sent = found->second->m_transport.send(found->second->m_remote, record);
    if (sent == NI_LIMIT_EXCEEDED) { return false; }
    if (sent != NI_OK) { m_local.close(local, sent); found->second->m_transport.close(found->second->m_remote, sent); }
    return true;
}

void NetClientOpaqueGateway::onRawLocalClosed(uint64_t local, int32_t reason)
{
    auto found = m_peers.find(local);
    if (found == m_peers.end()) { return; }
    found->second->m_closed = true;
    found->second->m_transport.close(found->second->m_remote, reason);
    // TCP回调正在执行时不能析构同一运输层；poll外层统一回收。
}

void NetClientOpaqueGateway::remoteClosed(uint64_t local, int32_t reason)
{
    const auto found = m_peers.find(local);
    // 本地正常关闭会同步收敛远端TCP；这不是入口故障，不能让后来的客户端误入冷却。
    if (found == m_peers.end() || found->second->m_closed) { return; }
    // Record the terminal state before the local close callback can re-enter us.
    found->second->m_closed = true;
    if (!m_stopping) { m_discovery.failed(found->second->m_address); }
    m_local.close(local, reason < 0 ? reason : NI_TRUNCATED);
}

bool NetClientOpaqueGateway::poll()
{
    if (m_stopping)
    {
        m_discovery.poll();
        return !m_discovery.stopped();
    }
    if (!m_started)
    {
        return false;
    }
    m_discovery.poll();
    m_local.poll();
    for (auto& entry : m_peers)
    {
        entry.second->m_transport.poll();
    }
    for (auto entry = m_peers.begin(); entry != m_peers.end();)
    {
        if (entry->second->m_closed)
        {
            entry = m_peers.erase(entry);
        }
        else { ++entry; }
    }
    return true;
}

void NetClientOpaqueGateway::shutdown()
{
    if (m_stopping) { return; }
    m_stopping = true;
    m_discovery.shutdown();
    m_local.shutdown();
    for (auto& entry : m_peers) { entry.second->m_transport.shutdown(); }
    m_peers.clear();
    if (m_receiving)
    {
        ProcessWork::instance().uninitReceive();
        m_receiving = false;
    }
}