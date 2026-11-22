#include "NetGatewayDiscovery.h"
#include "NetPeerMacro.h"
#include "NetGatewayDiscoveryState.h"
#include "NetDirectorySnapshot.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "NetTransportEntropy.h"
#include <chrono>
#include <cstring>

/** 发现期限采用单调时钟
@return 单调毫秒
*/
static uint64_t DiscoveryNow()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

NetGatewayDiscovery::NetGatewayDiscovery() :
m_state(new NetGatewayDiscoveryState())
{

}

NetGatewayDiscovery::~NetGatewayDiscovery()
{
    shutdown();
    delete m_state;
}

int32_t NetGatewayDiscovery::parseSeeds(const std::string& text, std::vector<NetGatewayAddress>& points)
{
    // 本模块拥有输出容器，通过值数组入口跨模块复制，不转交STL分配责任。
    uint32_t count = 0;
    const int32_t measured = NetClientDiscovery::parseSeeds(text, nullptr, 0, count);
    if (measured != NI_BUFFER_TOO_SMALL) { points.clear(); return measured; }
    try
    {
        std::vector<NetClientDiscoveryPoint> parsed(count);
        for (auto& point : parsed) { point.m_structSize = sizeof(point); point.m_version = 1; }
        const int32_t result = NetClientDiscovery::parseSeeds(text, parsed.data(), count, count);
        if (result != NI_OK) { points.clear(); return result; }
        std::vector<NetGatewayAddress> copied;
        copied.reserve(count);
        for (const auto& input : parsed)
        {
            NetGatewayAddress point;
            point.m_address = input.m_address;
            point.m_port = input.m_port;
            copied.push_back(point);
        }
        points.swap(copied);
        return NI_OK;
    }
    catch (...) { points.clear(); return NI_NO_MEMORY; }
}

int32_t NetGatewayDiscovery::start(const std::vector<NetGatewayAddress>& seeds, const std::string& cachePath,
    const NetInterfaceTlsOptions& tls)
{
    if (!stopped())
    {
        return NI_INVALID_STATE;
    }
    if (seeds.empty() || seeds.size() > UINT32_MAX)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        std::vector<NetClientDiscoveryPoint> copied(seeds.size());
        for (uint32_t index = 0; index < seeds.size(); ++index)
        {
            const NetGatewayAddress& seed = seeds[index];
            if (seed.m_address.size() >= sizeof(copied[index].m_address) || seed.m_address.find('\0') != std::string::npos)
            {
                return NI_INVALID_ARGUMENT;
            }
            copied[index].m_structSize = sizeof(copied[index]);
            copied[index].m_version = 1;
            copied[index].m_port = seed.m_port;
            std::memcpy(copied[index].m_address, seed.m_address.c_str(), seed.m_address.size() + 1);
            if (!NetClientDiscovery::validPoint(copied[index]))
            {
                return NI_INVALID_ARGUMENT;
            }
        }
        m_state->m_tls = NetInterfaceTlsOptions();
        m_state->m_tls.entropy = tls.entropy == nullptr ? NetTransportEntropy : tls.entropy;
        m_state->m_tls.entropyContext = tls.entropy == nullptr ? nullptr : tls.entropyContext;
        m_state->m_tls.handshakeTimeoutMs = tls.handshakeTimeoutMs == 0 ? 5000 : tls.handshakeTimeoutMs;
        m_state->m_seedCount = static_cast<uint32_t>(seeds.size());
        m_state->m_seeds.swap(copied);
        m_state->m_nodes.clear();
        m_state->m_nodeCount = m_state->m_candidateCount = 0;
        m_state->m_cacheEnabled = false;
        if (!cachePath.empty())
        {
            const int32_t opened = m_state->m_cache.open(cachePath, m_state->m_tls.entropy, m_state->m_tls.entropyContext);
            if (opened != NI_OK)
            {
                shutdown();
                return opened;
            }
            m_state->m_cacheEnabled = true;
        }
        const int32_t rebuilt = m_state->rebuild();
        if (rebuilt != NI_OK) { shutdown(); return rebuilt; }
        m_state->m_next = 0;
        m_state->m_retryAt = 0;
        m_state->m_running = true;
        m_state->m_cached = true;
        return NI_OK;
    }
    catch (...)
    {
        shutdown();
        return NI_NO_MEMORY;
    }
}

bool NetGatewayDiscovery::choose(NetClientDiscoveryPoint& endpoint)
{
    if (endpoint.m_structSize != sizeof(endpoint) || endpoint.m_version != 1)
    {
        return false;
    }
    std::memset(endpoint.m_address, 0, sizeof(endpoint.m_address));
    endpoint.m_port = 0;
    if (!m_state->m_running || m_state->m_candidateCount == 0)
    {
        return false;
    }
    const uint64_t now = DiscoveryNow();
    for (uint32_t index = 0; index < m_state->m_candidateCount; ++index)
    {
        const NetGatewayDiscoveryCandidate& candidate = m_state->m_candidates[m_state->m_next];
        m_state->m_next = (m_state->m_next + 1) % m_state->m_candidateCount;
        if (candidate.m_failedUntil != UINT64_MAX && now >= candidate.m_failedUntil)
        {
            endpoint = candidate.m_point;
            return true;
        }
    }
    return false;
}

void NetGatewayDiscovery::failed(const NetClientDiscoveryPoint& endpoint)
{
    uint8_t key[16];
    if (!m_state->m_running || !NetClientDiscovery::addressKey(endpoint, key))
    {
        return;
    }
    for (uint32_t index = 0; index < m_state->m_candidateCount; ++index)
    {
        NetGatewayDiscoveryCandidate& candidate = m_state->m_candidates[index];
        if (candidate.m_point.m_port == endpoint.m_port && std::memcmp(candidate.m_key, key, 16) == 0)
        {
            uint32_t random = 0;
            if (NetInterface::entropyBytes(m_state->m_tls.entropy, m_state->m_tls.entropyContext, reinterpret_cast<uint8_t*>(&random), sizeof(random)) != NI_OK)
            {
                candidate.m_failedUntil = UINT64_MAX;
                return;
            }
            const uint64_t now = DiscoveryNow();
            const uint32_t delay = 500 + random % 1000;
            candidate.m_failedUntil = now > UINT64_MAX - delay ? UINT64_MAX : now + delay;
            break;
        }
    }
}

void NetGatewayDiscovery::onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity)
{
    if (peer != m_state->m_peer || identity.m_role != NI_LINK_NODE) { m_state->m_link->close(peer, NI_PROTOCOL_ERROR); return; }
    m_state->m_ready = true;
}

bool NetGatewayDiscovery::onLinkControl(uint64_t peer, const NetInterfaceFrame& frame)
{
    if (m_state->m_closed || peer != m_state->m_peer) { return true; }
    if (frame.m_kind != NI_RESPONSE || frame.m_productId != 0 || frame.m_messageType != NI_GATEWAY_DISCOVER ||
        frame.m_requestId != m_state->m_waiting || m_state->m_waiting == 0) { m_state->m_link->close(peer, NI_PROTOCOL_ERROR); return true; }
    NetDirectorySnapshot snapshot;
    std::vector<NetInterfaceDirectoryNodeData> points;
    if (snapshot.parse(frame.m_payload, frame.m_payloadSize) != NI_OK || snapshot.values(points) != NI_OK)
    {
        m_state->m_link->close(peer, NI_PROTOCOL_ERROR);
        return true;
    }
    m_state->m_nodes.swap(points);
    const int32_t rebuilt = m_state->rebuild();
    if (rebuilt != NI_OK)
    {
        m_state->m_nodes.swap(points);
        m_state->m_link->close(peer, rebuilt);
        return true;
    }
    m_state->m_nodeCount = static_cast<uint32_t>(m_state->m_nodes.size());
    m_state->m_cached = false;
    if (m_state->m_cacheEnabled) { m_state->m_cache.queue(m_state->m_nodes.data(), m_state->m_nodeCount); }
    m_state->m_waiting = 0;
    return true;
}

bool NetGatewayDiscovery::onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record)
{
    (void)record;
    m_state->m_link->close(peer, NI_PROTOCOL_ERROR); // 只发现的管理连接没有业务通道。
    return true;
}

void NetGatewayDiscovery::onLinkClosed(uint64_t peer, int32_t reason)
{
    (void)reason;
    if (peer != m_state->m_peer) { return; }
    m_state->m_ready = false;
    m_state->m_closed = true;
    if (m_state->m_running) { failed(m_state->m_current); m_state->m_retryAt = DiscoveryNow() + 250; }
}

bool NetGatewayDiscovery::poll()
{
    if (!m_state->m_running)
    {
        m_state->m_cache.poll();
        return !stopped();
    }
    try
    {
        if (m_state->m_cacheEnabled)
        {
            m_state->m_cache.poll();
            uint32_t count = 0;
            if (m_state->m_cache.takeLoaded(nullptr, 0, count) == NI_BUFFER_TOO_SMALL && count > 0)
            {
                std::vector<NetInterfaceDirectoryNodeData> cached(count);
                for (auto& node : cached)
                {
                    node.m_structSize = sizeof(node);
                    node.m_version = 1;
                }
                if (m_state->m_cache.takeLoaded(cached.data(), count, count) == NI_READY && m_state->m_cached)
                {
                    m_state->m_nodes.swap(cached);
                    if (m_state->rebuild() == NI_OK) { m_state->m_nodeCount = count; }
                    else { m_state->m_nodes.swap(cached); }
                }
            }
        }
    }
    catch (...) { m_state->m_cacheEnabled = false; m_state->m_cache.shutdown(); }
    if (m_state->m_link) { m_state->m_link->poll(); }
    const uint64_t now = DiscoveryNow();
    if (m_state->m_closed && m_state->m_link)
    {
        m_state->m_link->shutdown();
        m_state->m_link.reset();
        m_state->m_peer = m_state->m_waiting = m_state->m_request = m_state->m_sentAt = 0;
        m_state->m_closed = false;
    }
    if (!m_state->m_link && now >= m_state->m_retryAt)
    {
        if (!choose(m_state->m_current)) { m_state->m_retryAt = now + 250; return true; }
        try
        {
            m_state->m_link.reset(new NetLinkTransport());
        }
        catch (...)
        {
            failed(m_state->m_current);
            m_state->m_retryAt = now + 250;
            return true;
        }
        NetInterfaceLinkOptions options = {};
        options.m_version = 3;
        options.m_role = NI_LINK_CLIENT;
        options.m_tls = m_state->m_tls;
        const int32_t result = m_state->m_link->connect(m_state->m_current.m_address, m_state->m_current.m_port, options, this, m_state->m_peer);
        if (result != NI_OK) { m_state->m_closed = true; failed(m_state->m_current); m_state->m_retryAt = now + 250; }
    }
    if (m_state->m_ready && m_state->m_link && !m_state->m_closed)
    {
        if (m_state->m_waiting != 0 && now - m_state->m_sentAt >= 5000) { m_state->m_link->close(m_state->m_peer, NI_TIMEOUT); }
        else if (m_state->m_waiting == 0 && (m_state->m_sentAt == 0 || now - m_state->m_sentAt >= 5000))
        {
            if (m_state->m_request == UINT64_MAX) { m_state->m_link->close(m_state->m_peer, NI_LIMIT_EXCEEDED); return true; }
            NetInterfaceFrame frame = {};
            frame.m_kind = NI_REQUEST;
            frame.m_messageType = NI_GATEWAY_DISCOVER;
            frame.m_requestId = m_state->m_request + 1;
            const int32_t sent = m_state->m_link->sendControl(m_state->m_peer, frame);
            if (sent == NI_OK) { m_state->m_request = m_state->m_waiting = frame.m_requestId; m_state->m_sentAt = now; }
            else if (sent != NI_LIMIT_EXCEEDED) { m_state->m_link->close(m_state->m_peer, sent); }
        }
    }
    return true;
}

uint32_t NetGatewayDiscovery::discovered() const
{
    return m_state->m_cached ? 0 : m_state->m_nodeCount;
}

void NetGatewayDiscovery::shutdown()
{
    m_state->m_running = false;
    if (m_state->m_link) { m_state->m_link->shutdown(); m_state->m_link.reset(); }
    m_state->m_cache.shutdown();
    m_state->m_cache.poll();
    m_state->m_cacheEnabled = false;
    m_state->m_ready = m_state->m_closed = false;
    m_state->m_peer = m_state->m_request = m_state->m_waiting = m_state->m_sentAt = 0;
    m_state->m_seeds.clear();
    m_state->m_nodes.clear();
    m_state->m_candidates.clear();
    m_state->m_seedCount = m_state->m_nodeCount = m_state->m_candidateCount = 0;
    m_state->m_next = 0;
    m_state->m_cached = false;
}

bool NetGatewayDiscovery::stopped() const
{
    return !m_state->m_running && !m_state->m_link && m_state->m_cache.stopped();
}

// 仅测试时放开此main，完成后逐行注释；不得替代真实TLS目录集成测试