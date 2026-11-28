#include "NetGatewayDiscovery.h"
#include "NetPeerMacro.h"
#include "NetGatewayDiscoveryState.h"
#include "NetDirectorySnapshot.h"
#include "NetInterface/NetInterfaceAPI.h"
#include "NetTransportEntropy.h"
#include "CStringManager/CStringManagerAPI.h"
#include <cstring>

/** 发现期限采用单调时钟
@return 单调毫秒
*/
static uint64_t DiscoveryNow()
{
    return NetInterface::monotonicMilliseconds();
}

NetGatewayDiscovery::NetGatewayDiscovery() : NetGatewayDiscovery(NetGatewayDiscoveryConfig())
{
}

NetGatewayDiscovery::NetGatewayDiscovery(const NetGatewayDiscoveryConfig& config) :
m_state(new NetGatewayDiscoveryState(config))
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
    NetInterfaceTlsOptions discoveryTls = {};
    if (!m_state->m_config.valid() || !discoveryTls.setHandshakeTimeout(tls.handshakeTimeoutMs == 0 ? 5000 : tls.handshakeTimeoutMs) ||
        seeds.empty() || seeds.size() > UINT32_MAX)
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        std::vector<NetClientDiscoveryPoint> copied(seeds.size());
        for (uint32_t index = 0; index < seeds.size(); ++index)
        {
            const NetGatewayAddress& seed = seeds[index];
            if (seed.m_address.size() >= sizeof(copied[index].m_address) ||
                CStringManager::boundedLength(seed.m_address.c_str(), seed.m_address.size()) != seed.m_address.size())
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
        discoveryTls.entropy = tls.entropy == nullptr ? NetTransportEntropy : tls.entropy;
        discoveryTls.entropyContext = tls.entropy == nullptr ? nullptr : tls.entropyContext;
        m_state->m_tls = discoveryTls;
        m_state->m_seeds.swap(copied);
        m_state->m_nodes.clear();
        if (!cachePath.empty())
        {
            const int32_t opened = m_state->m_cache.open(cachePath, m_state->m_tls.entropy, m_state->m_tls.entropyContext);
            if (opened != NI_OK)
            {
                shutdown();
                return opened;
            }
        }
        const int32_t rebuilt = m_state->rebuild();
        if (rebuilt != NI_OK) { shutdown(); return rebuilt; }
        m_state->m_next = 0;
        m_state->m_retryAt = 0;
        m_state->m_running = true;
        m_state->m_cached = true;
        NetExecution::progress(); // 第一轮 poll 才会创建探测连接。
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
    if (!m_state->m_running || m_state->m_candidates.empty())
    {
        return false;
    }
    const uint64_t now = DiscoveryNow();
    for (size_t index = 0; index < m_state->m_candidates.size(); ++index)
    {
        const NetGatewayDiscoveryCandidate& candidate = m_state->m_candidates[m_state->m_next];
        m_state->m_next = (m_state->m_next + 1) % m_state->m_candidates.size();
        if (now >= candidate.m_failedUntil)
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
    for (auto& candidate : m_state->m_candidates)
    {
        if (candidate.m_point.m_port == endpoint.m_port && std::memcmp(candidate.m_key, key, 16) == 0)
        {
            uint64_t delay = m_state->m_config.m_backoffMs;
            const uint32_t range = m_state->m_config.m_jitterMs;
            if (range > 1)
            {
                uint32_t random = 0;
                const int32_t result = NetInterface::entropyBytes(m_state->m_tls.entropy, m_state->m_tls.entropyContext,
                    reinterpret_cast<uint8_t*>(&random), sizeof(random));
                // 抖动取样失败时等满允许的间隔；下一次连接仍须取得真实握手熵。
                delay += result == NI_OK ? random % range : range - 1;
            }
            const uint64_t now = DiscoveryNow();
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
    const int32_t parsed = snapshot.parse(frame.m_payload, frame.m_payloadSize);
    const int32_t loaded = parsed == NI_OK ? snapshot.values(points) : parsed;
    if (loaded != NI_OK)
    {
        // 解码与复制都可能耗尽本地内存；保留该原因，其他无效目录按远端协议错误关闭。
        m_state->m_link->close(peer, loaded == NI_NO_MEMORY ? NI_NO_MEMORY : NI_PROTOCOL_ERROR);
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
    m_state->m_cached = false;
    if (!m_state->m_cache.stopped())
    {
        m_state->m_cache.queue(m_state->m_nodes.data(), static_cast<uint32_t>(m_state->m_nodes.size()));
    }
    m_state->m_waiting = 0;
    m_state->m_sentAt = DiscoveryNow();
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
    if (m_state->m_running) { failed(m_state->m_current); m_state->m_retryAt = DiscoveryNow() + m_state->m_config.m_retryMs; }
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
        if (!m_state->m_cache.stopped())
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
                    if (m_state->rebuild() != NI_OK) { m_state->m_nodes.swap(cached); }
                }
            }
        }
    }
    catch (...)
    {
        // 缓存分配失败后仍由后续 poll 推进宿主释放，目录探测继续使用已知候选。
        m_state->m_cache.shutdown();
    }
    if (m_state->m_link) { m_state->m_link->poll(); }
    const uint64_t now = DiscoveryNow();
    if (m_state->m_closed && m_state->m_link)
    {
        m_state->m_link->shutdown();
        m_state->m_link.reset();
        m_state->m_peer = m_state->m_waiting = m_state->m_request = m_state->m_sentAt = 0;
        m_state->m_closed = false;
    }
    if (!m_state->m_link)
    {
        if (now < m_state->m_retryAt)
        {
            NetExecution::deadline(m_state->m_retryAt, now);
            return true;
        }
        if (!choose(m_state->m_current))
        {
            m_state->m_retryAt = now + m_state->m_config.m_retryMs;
            NetExecution::deadline(m_state->m_retryAt, now);
            return true;
        }
        try
        {
            m_state->m_link.reset(new NetLinkTransport(m_state->m_config.m_transport));
        }
        catch (...)
        {
            failed(m_state->m_current);
            m_state->m_retryAt = now + m_state->m_config.m_retryMs;
            NetExecution::deadline(m_state->m_retryAt, DiscoveryNow());
            return true;
        }
        NetInterfaceLinkOptions options = {};
        options.m_version = 3;
        options.m_role = NI_LINK_CLIENT;
        options.m_tls = m_state->m_tls;
        const int32_t result = m_state->m_link->connect(m_state->m_current.m_address, m_state->m_current.m_port, options, this, m_state->m_peer);
        if (result != NI_OK) { m_state->m_closed = true; failed(m_state->m_current); m_state->m_retryAt = now + m_state->m_config.m_retryMs; }
        // 下一轮首次推进新建的运输，或回收本次未成功启动的运输。
        NetExecution::progress();
    }
    if (m_state->m_ready && m_state->m_link && !m_state->m_closed)
    {
        if (m_state->m_waiting != 0 && now - m_state->m_sentAt >= m_state->m_config.m_responseMs) { m_state->m_link->close(m_state->m_peer, NI_TIMEOUT); }
        else if (m_state->m_waiting == 0 && (m_state->m_sentAt == 0 || now - m_state->m_sentAt >= m_state->m_config.m_refreshMs))
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
        const uint32_t interval = m_state->m_waiting != 0 ? m_state->m_config.m_responseMs : m_state->m_config.m_refreshMs;
        const uint64_t elapsed = now - m_state->m_sentAt;
        if (elapsed < interval) { NetExecution::deadline(interval, elapsed); }
    }
    return true;
}

uint32_t NetGatewayDiscovery::discovered() const
{
    return m_state->m_cached ? 0 : static_cast<uint32_t>(m_state->m_nodes.size());
}

void NetGatewayDiscovery::shutdown()
{
    m_state->m_running = false;
    if (m_state->m_link) { m_state->m_link->shutdown(); m_state->m_link.reset(); }
    m_state->m_cache.shutdown();
    m_state->m_cache.poll();
    m_state->m_ready = m_state->m_closed = false;
    m_state->m_peer = m_state->m_request = m_state->m_waiting = m_state->m_sentAt = 0;
    m_state->m_seeds.clear();
    m_state->m_nodes.clear();
    m_state->m_candidates.clear();
    m_state->m_next = 0;
    m_state->m_cached = false;
}

bool NetGatewayDiscovery::stopped() const
{
    return !m_state->m_running && !m_state->m_link && m_state->m_cache.stopped();
}