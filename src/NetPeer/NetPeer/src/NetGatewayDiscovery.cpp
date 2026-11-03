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
    NetClientDiscoveryPoint parsed[64] = {};
    for (auto& point : parsed)
    {
        point.m_structSize = sizeof(point);
        point.m_version = 1;
    }
    uint32_t count = 0;
    const int32_t result = NetClientDiscovery::parseSeeds(text, parsed, 64, count);
    if (result != NI_OK)
    {
        points.clear();
        return result;
    }
    try
    {
        std::vector<NetGatewayAddress> copied;
        copied.reserve(count);
        for (uint32_t index = 0; index < count; ++index)
        {
            NetGatewayAddress point;
            point.m_address = parsed[index].m_address;
            point.m_port = parsed[index].m_port;
            copied.push_back(point);
        }
        points.swap(copied);
        return NI_OK;
    }
    catch (...)
    {
        points.clear();
        return NI_NO_MEMORY;
    }
}

int32_t NetGatewayDiscovery::start(const std::vector<NetGatewayAddress>& seeds, const std::string& cachePath,
    const NetInterfaceTlsOptions& tls)
{
    if (!stopped())
    {
        return NI_INVALID_STATE;
    }
    if (seeds.empty() || seeds.size() > 64)
    {
        return NI_INVALID_ARGUMENT;
    }
    NetClientDiscoveryPoint copied[64] = {};
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
    try
    {
        m_state->m_tls = NetInterfaceTlsOptions();
        m_state->m_tls.entropy = tls.entropy == nullptr ? NetTransportEntropy : tls.entropy;
        m_state->m_tls.entropyContext = tls.entropy == nullptr ? nullptr : tls.entropyContext;
        m_state->m_tls.handshakeTimeoutMs = tls.handshakeTimeoutMs == 0 ? 5000 : tls.handshakeTimeoutMs;
        m_state->m_seedCount = static_cast<uint32_t>(seeds.size());
        std::memcpy(m_state->m_seeds, copied, m_state->m_seedCount * sizeof(copied[0]));
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
        m_state->rebuild();
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
    NetInterfaceDirectoryNodeData points[64] = {};
    for (auto& point : points)
    {
        point.m_structSize = sizeof(point);
        point.m_version = 1;
    }
    uint32_t count = 0;
    if (snapshot.parse(frame.m_payload, frame.m_payloadSize) != NI_OK || snapshot.copy(points, 64, count) != NI_OK)
    {
        m_state->m_link->close(peer, NI_PROTOCOL_ERROR);
        return true;
    }
    std::memcpy(m_state->m_nodes, points, count * sizeof(points[0]));
    m_state->m_nodeCount = count;
    m_state->m_cached = false;
    m_state->rebuild();
    if (m_state->m_cacheEnabled) { m_state->m_cache.queue(m_state->m_nodes, count); }
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
    if (m_state->m_cacheEnabled)
    {
        m_state->m_cache.poll();
        uint32_t count = 0;
        if (m_state->m_cache.takeLoaded(nullptr, 0, count) == NI_BUFFER_TOO_SMALL && count > 0 && count <= 64)
        {
            NetInterfaceDirectoryNodeData cached[64] = {};
            for (auto& node : cached)
            {
                node.m_structSize = sizeof(node);
                node.m_version = 1;
            }
            if (m_state->m_cache.takeLoaded(cached, 64, count) == NI_READY && m_state->m_cached)
            {
                std::memcpy(m_state->m_nodes, cached, count * sizeof(cached[0]));
                m_state->m_nodeCount = count;
                m_state->rebuild();
            }
        }
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
    m_state->m_seedCount = m_state->m_nodeCount = m_state->m_candidateCount = 0;
    m_state->m_next = 0;
    m_state->m_cached = false;
}

bool NetGatewayDiscovery::stopped() const
{
    return !m_state->m_running && !m_state->m_link && m_state->m_cache.stopped();
}

// 仅测试时放开此main，完成后逐行注释；不得替代真实TLS目录集成测试
// NET_GATEWAY_DISCOVERY_UNIT_BEGIN
//#include "NetPeer.h"
//#include "NetInterface/NetInterfaceAPI.h"
//#include <cstdio>
//#include <cstring>
//#include <stdexcept>
//#if defined(STATIC_LIB)
//#include "NetPeer/NetGatewayDiscoveryState.h"
//#endif
//#if defined(_MSC_VER) && defined(_DEBUG)
//#include <crtdbg.h>
//#endif
//
//int main()
//{
//    uint32_t checks = 0;
//    const auto Check = [&checks](bool passed, const char* message)
//    {
//        ++checks;
//        if (!passed) { throw std::runtime_error(message); }
//    };
//    try
//    {
//        NetClientDiscoveryPoint output = {};
//        output.m_structSize = sizeof(output);
//        output.m_version = 1;
//        uint32_t count = 17;
//        Check(NetClientDiscovery::parseSeeds("[2001:db8::1]:7,[2001:0DB8:0:0:0:0:0:0001]:7", &output, 1, count) == NI_OK && count == 1,
//            "canonical seed parser");
//        uint8_t key[16] = {};
//        const uint8_t expected[16] = {0x20, 1, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
//        Check(NetClientDiscovery::addressKey(output, key) && std::memcmp(key, expected, 16) == 0, "independent IPv6 key");
//        Check(NetClientDiscovery::parseSeeds("127.0.0.1:12345", &output, 1, count) == NI_OK, "IPv4 key fixture");
//        const uint8_t mapped[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 127, 0, 0, 1};
//        Check(NetClientDiscovery::addressKey(output, key) && std::memcmp(key, mapped, 16) == 0, "independent IPv4 mapped key");
//        const NetClientDiscoveryPoint kept = output;
//        Check(NetClientDiscovery::parseSeeds("127.0.0.2:9", &output, 1, output.m_port) == NI_INVALID_ARGUMENT &&
//            std::memcmp(&kept, &output, sizeof(output)) == 0, "count overlaps output before writes");
//        std::string text = "127.0.0.1:9";
//        count = 42;
//        Check(NetClientDiscovery::parseSeeds(text, reinterpret_cast<NetClientDiscoveryPoint*>(&text), 1, count) == NI_INVALID_ARGUMENT &&
//            text == "127.0.0.1:9" && count == 42, "output overlaps string object");
//        output.m_port = 0;
//        Check(!NetClientDiscovery::addressKey(output, key) && std::memcmp(key, mapped, 16) == 0, "invalid key changed output");
//        output = kept;
//        Check(!NetClientDiscovery::addressKey(output, nullptr), "null key");
//        std::memset(output.m_address, '1', sizeof(output.m_address));
//        Check(!NetClientDiscovery::addressKey(output, key), "unbounded address");
//        output = kept;
//
//#if defined(STATIC_LIB)
//        {
//            NetGatewayDiscoveryState table;
//            table.m_seedCount = table.m_nodeCount = 64;
//            for (uint32_t index = 0; index < 64; ++index)
//            {
//                table.m_seeds[index] = kept;
//                table.m_seeds[index].m_port = 10000 + index;
//                table.m_nodes[index] = NetInterfaceDirectoryNodeData();
//                std::strcpy(table.m_nodes[index].m_address, "::ffff:7f00:1");
//                table.m_nodes[index].m_port = 20000 + index;
//            }
//            table.rebuild();
//            Check(table.m_candidateCount == 128, "full candidate bound");
//            for (uint32_t index = 0; index < 128; ++index)
//            {
//                Check(table.m_candidates[index].m_point.m_port == (index < 64 ? 10000 + index : 20000 + index - 64) &&
//                    table.m_candidates[index].m_failedUntil == 0, "full candidate ordering");
//                table.m_candidates[index].m_failedUntil = index == 63 ? UINT64_MAX : 5000 + index;
//            }
//            for (uint32_t index = 0; index < 64; ++index)
//            {
//                table.m_nodes[index].m_port = 10032 + index;
//            }
//            table.m_next = UINT32_MAX;
//            table.rebuild();
//            Check(table.m_candidateCount == 96 && table.m_next < 96, "mapped dedup/cursor bound");
//            for (uint32_t index = 0; index < 96; ++index)
//            {
//                Check(table.m_candidates[index].m_point.m_port == 10000 + index, "merge dropped seed or new node");
//                Check(table.m_candidates[index].m_failedUntil == (index == 63 ? UINT64_MAX : index < 64 ? 5000 + index : 0),
//                    "numeric key did not retain failure deadline");
//            }
//            table.m_nodeCount = 0;
//            table.rebuild();
//            Check(table.m_candidateCount == 64, "empty directory removed seeds");
//            table.m_nodeCount = 1;
//            table.m_nodes[0].m_port = 20000;
//            table.rebuild();
//            Check(table.m_candidateCount == 65 && table.m_candidates[64].m_failedUntil == 0, "removed node retained stale failure");
//        }
//        std::vector<NetGatewayAddress> alias(1);
//        alias[0].m_address = "127.0.0.1:12345,[2001:db8::1]:23456";
//        Check(NetGatewayDiscovery::parseSeeds(alias[0].m_address, alias) == NI_OK && alias.size() == 2 &&
//            alias[0].m_address == "127.0.0.1" && alias[1].m_port == 23456, "aliased wrapper input");
//        Check(NetGatewayDiscovery::parseSeeds("bad", alias) == NI_INVALID_ARGUMENT && alias.empty(), "invalid wrapper clears output");
//#endif
//        NetInterfaceTlsOptions tls = {};
//        uint32_t entropyCalls = 0;
//        tls.entropyContext = &entropyCalls;
//        tls.entropy = [](void* context, uint8_t* bytes, uint32_t size) -> int32_t
//        {
//            ++*static_cast<uint32_t*>(context);
//            std::memset(bytes, 0, size);
//            return NI_OK;
//        };
//        std::vector<NetGatewayAddress> seeds(4);
//        seeds[0].m_address = "2001:0db8:0000:0000:0000:0000:0000:0001";
//        seeds[0].m_port = 12345;
//        seeds[1].m_address = "2001:db8::1";
//        seeds[1].m_port = 12345;
//        seeds[2].m_address = "127.0.0.1";
//        seeds[2].m_port = 23456;
//        seeds[3].m_address = "::ffff:7f00:1";
//        seeds[3].m_port = 23456;
//        NetGatewayDiscovery discovery;
//        Check(discovery.stopped() && !discovery.poll() && !discovery.choose(output), "new discovery resources");
//        Check(discovery.start(seeds, "", tls) == NI_OK && !discovery.stopped() && discovery.discovered() == 0, "start seed hints");
//        Check(discovery.start(seeds, "", tls) < 0, "second start accepted");
//        for (uint32_t index = 0; index < 1000; ++index)
//        {
//            Check(discovery.choose(output) && output.m_port == (index % 2 == 0 ? 12345 : 23456), "canonical rotation");
//            Check(std::strcmp(output.m_address, seeds[index % 2 == 0 ? 0 : 2].m_address.c_str()) == 0, "first seed spelling");
//        }
//        std::strcpy(output.m_address, "2001:DB8::1");
//        output.m_port = 12345;
//        discovery.failed(output);
//        Check(entropyCalls == 1, "failure not found by key");
//        Check(discovery.choose(output) && output.m_port == 23456, "failed duplicate selected");
//        output.m_port = 30000;
//        discovery.failed(output);
//        Check(entropyCalls == 1, "unknown endpoint consumes entropy");
//        discovery.shutdown();
//        discovery.shutdown();
//        Check(discovery.stopped() && !discovery.poll() && !discovery.choose(output) && discovery.discovered() == 0, "stop not idempotent");
//
//        tls.entropy = [](void* context, uint8_t*, uint32_t) -> int32_t
//        {
//            ++*static_cast<uint32_t*>(context);
//            throw 7;
//        };
//        for (uint32_t cycle = 0; cycle < 100; ++cycle)
//        {
//            Check(discovery.start(seeds, "", tls) == NI_OK, "restart");
//            for (uint32_t index = 0; index < 2; ++index)
//            {
//                Check(discovery.choose(output), "restart did not clear backoff");
//                discovery.failed(output);
//            }
//            Check(!discovery.choose(output), "entropy exception failed open");
//            discovery.shutdown();
//            Check(discovery.stopped(), "restart resource leak");
//        }
//
//        seeds.resize(64);
//        for (uint32_t index = 0; index < 64; ++index)
//        {
//            seeds[index].m_address = "2001:0db8:0000:0000:0000:0000:0000:0001";
//            seeds[index].m_port = 10000 + index;
//        }
//        Check(discovery.start(seeds, "", tls) == NI_OK, "64 seeds rejected");
//#if defined(_MSC_VER) && defined(_DEBUG)
//        static uint32_t s_allocations = 0;
//        const auto Hook = [](int operation, void*, size_t, int block, long, const unsigned char*, int) -> int
//        {
//            if (operation == _HOOK_ALLOC && block == _NORMAL_BLOCK) { ++s_allocations; }
//            return 1;
//        };
//        _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(Hook);
//        bool ordered = true;
//        for (uint32_t index = 0; index < 64000; ++index)
//        {
//            ordered = discovery.choose(output) && output.m_port == 10000 + index % 64 && ordered;
//        }
//        _CrtSetAllocHook(previous);
//        Check(ordered && s_allocations == 0, "choose allocated or broke rotation");
//#else
//        for (uint32_t index = 0; index < 64000; ++index)
//        {
//            Check(discovery.choose(output) && output.m_port == 10000 + index % 64, "64 candidate rotation");
//        }
//#endif
//        for (uint32_t index = 0; index < 64; ++index)
//        {
//            Check(discovery.choose(output) && output.m_port == 10000 + index, "failure distribution");
//            discovery.failed(output);
//        }
//        Check(!discovery.choose(output), "blocked table yields candidate");
//        discovery.shutdown();
//        seeds.push_back(seeds[0]);
//        Check(discovery.start(seeds, "", tls) == NI_INVALID_ARGUMENT && discovery.stopped(), "65 seeds accepted");
//        seeds.resize(1);
//        const char* invalid[] = {"", "0.0.0.0", "::", "255.255.255.255", "localhost", "[::1]", "::1%lo", "1.2.3.4x"};
//        for (uint32_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]); ++index)
//        {
//            seeds[0].m_address = invalid[index];
//            Check(discovery.start(seeds, "", tls) == NI_INVALID_ARGUMENT && discovery.stopped(), "invalid seed address");
//        }
//        seeds[0].m_address.assign("127.0.0.1\0x", 11);
//        Check(discovery.start(seeds, "", tls) == NI_INVALID_ARGUMENT, "embedded NUL seed");
//        seeds[0].m_address = "127.0.0.1";
//        seeds[0].m_port = 0;
//        Check(discovery.start(seeds, "", tls) == NI_INVALID_ARGUMENT, "zero port");
//        seeds[0].m_port = 65536;
//        Check(discovery.start(seeds, "", tls) == NI_INVALID_ARGUMENT, "oversized port");
//        seeds.clear();
//        Check(discovery.start(seeds, "", tls) == NI_INVALID_ARGUMENT, "empty seeds");
//        std::printf("Discovery bounded selection checks=%u PASS\n", checks);
//        return 0;
//    }
//    catch (const std::exception& error)
//    {
//        std::printf("Discovery checks=%u FAIL %s\n", checks, error.what());
//    }
//    catch (...)
//    {
//        std::printf("Discovery checks=%u FAIL escaped exception\n", checks);
//    }
//    return 1;
//}
// NET_GATEWAY_DISCOVERY_UNIT_END