#include "NetInterfaceConnectionTestCases.h"
#include "NetInterfaceConnectionTestCasesHelper.h"
#include <memory>
#include "NetInterface/NetInterfaceAPI.h"
#include "NetPeer/NetPeerAPI.h"
#include "NetInterfaceTlsTestCases.h"
#include "NetWorkTestCases.h"
#include "CSystem/CSystemAPI.h"
#include <chrono>
#include <algorithm>
#include <cstring>
#include <stdexcept>

/** 转移不规则密文块，不读取或生成明文
@param [in,out] source 实际连接输出
@param [in,out] destination 实际连接输入
*/
static void TransferConnection(NetInterfaceConnection& source, NetInterfaceConnection& destination)
{
    const uint8_t* bytes = nullptr;
    uint32_t size = 0;
    source.peekOutput(bytes, size);
    uint32_t used = 0;
    NetInterfaceTlsTestPair::require(destination.feed(bytes, (std::min)(size, 733U), used) >= 0,
        "connection rejected ciphertext");
    source.consumeOutput(used);
}

void NetInterfaceConnectionTestCases::discoverySelection()
{
    NetGatewayDiscovery discovery;
    NetClientDiscoveryPoint point = {};
    point.m_structSize = sizeof(point);
    point.m_version = 1;
    std::vector<NetGatewayAddress> seeds(1);
    seeds[0].m_address = "127.0.0.1:12345,[::ffff:7f00:1]:12345,[2001:db8::1]:12345";
    NetInterfaceTlsTestPair::require(NetGatewayDiscovery::parseSeeds(seeds[0].m_address, seeds) == NI_OK && seeds.size() == 2,
        "discovery wrapper alias or numeric key failed");
    NetInterfaceTlsOptions tls = {};
    uint32_t entropyCalls = 0;
    tls.entropyContext = &entropyCalls;
    tls.entropy = [](void* context, uint8_t*, uint32_t) -> int32_t
    {
        ++*static_cast<uint32_t*>(context);
        return NI_INVALID_STATE;
    };
    for (uint32_t cycle = 0; cycle < 100; ++cycle)
    {
        netTestCheckpoint();
        NetInterfaceTlsTestPair::require(discovery.start(seeds, "", tls) == NI_OK && discovery.discovered() == 0,
            "discovery restart or unverified seed promoted");
        for (uint32_t index = 0; index < 20; ++index)
        {
            NetInterfaceTlsTestPair::require(discovery.choose(point) && std::strcmp(point.m_address, seeds[index % 2].m_address.c_str()) == 0,
                "seed round robin changed order");
        }
        std::strcpy(point.m_address, "::ffff:7f00:1");
        point.m_port = 12345;
        discovery.failed(point);
        NetInterfaceTlsTestPair::require(discovery.choose(point) && std::strcmp(point.m_address, "2001:db8::1") == 0,
            "equivalent failed address was selected again");
        discovery.failed(point);
        NetInterfaceTlsTestPair::require(!discovery.choose(point) && entropyCalls == (cycle + 1) * 2,
            "failed entropy allows candidate or unknown state");
        discovery.shutdown();
        NetInterfaceTlsTestPair::require(discovery.stopped() && !discovery.poll() && !discovery.choose(point),
            "stopped discovery keeps resources");
    }
    seeds.resize(64);
    for (uint32_t index = 0; index < 64; ++index)
    {
        seeds[index].m_address = "127.0.0.1";
        seeds[index].m_port = 10000 + index;
    }
    NetInterfaceTlsTestPair::require(discovery.start(seeds, "", tls) == NI_OK, "64 seed discovery start");
    for (uint32_t index = 0; index < 6400; ++index)
    {
        NetInterfaceTlsTestPair::require(discovery.choose(point) && point.m_port == 10000 + index % 64, "64 seed rotation");
    }
    discovery.shutdown();
    seeds.push_back(seeds[0]);
    NetInterfaceTlsTestPair::require(discovery.start(seeds, "", tls) == NI_INVALID_ARGUMENT && discovery.stopped(), "65 seeds accepted");
    point.m_port = 12345;
    const NetClientDiscoveryPoint original = point;
    NetInterfaceTlsTestPair::require(NetClientDiscovery::parseSeeds("127.0.0.2:9", &point, 1, point.m_port) == NI_INVALID_ARGUMENT &&
        std::memcmp(&point, &original, sizeof(point)) == 0, "count/output alias corrupted point");
}

/** 完成双方实际TLS握手，未借测试标志改变状态
@param [in,out] client 客户端
@param [in,out] server 服务端
*/
static void HandshakeConnection(NetInterfaceConnection& client, NetInterfaceConnection& server)
{
    for (uint32_t step = 0; step < 20000; ++step)
    {
        netTestCheckpoint();
        NetInterfaceTlsTestPair::require(client.progress(1000) >= 0 && server.progress(1000) >= 0,
            "combined TLS handshake failed");
        TransferConnection(client, server);
        TransferConnection(server, client);
        if (client.status() == NET_INTERFACE_TLS_READY && server.status() == NET_INTERFACE_TLS_READY)
        {
            return;
        }
    }
    throw std::runtime_error("combined handshake step bound");
}

void NetInterfaceConnectionTestCases::framedTransfer()
{
    NetInterfaceConnection client(NetInterfaceTlsTestPair::options(false, "localhost"));
    NetInterfaceConnection server(NetInterfaceTlsTestPair::options(true, nullptr));
    HandshakeConnection(client, server);
    std::vector<uint8_t> payload(1048576);
    for (size_t index = 0; index < payload.size(); ++index)
    {
        payload[index] = static_cast<uint8_t>(index * 37 + 11);
    }
    NetInterfaceFrame outgoing = {};
    outgoing.m_kind = NI_REQUEST;
    outgoing.m_productId = 5;
    outgoing.m_requestId = 0xfedcba9876543210ULL;
    outgoing.m_messageType = 29;
    outgoing.m_payload = &payload[0];
    outgoing.m_payloadSize = static_cast<uint32_t>(payload.size());
    NetInterfaceTlsTestPair::require(client.send(outgoing) == NI_OK, "combined large frame queue");
    outgoing.m_requestId = 42;
    outgoing.m_payloadSize = 3;
    NetInterfaceTlsTestPair::require(client.send(outgoing) == NI_OK, "combined next frame queue");
    NetInterfaceFrame incoming = {};
    while (server.peek(incoming) != NI_READY)
    {
        netTestCheckpoint();
        client.progress(1000);
        TransferConnection(client, server);
        NetInterfaceTlsTestPair::require(server.progress(1000) >= 0, "combined frame receive");
    }
    NetInterfaceTlsTestPair::require(incoming.m_requestId == 0xfedcba9876543210ULL &&
        incoming.m_payloadSize == payload.size() && std::memcmp(incoming.m_payload, &payload[0], payload.size()) == 0,
        "combined large frame bytes or identity");
    const uint8_t* borrowed = incoming.m_payload;
    for (int step = 0; step < 100; ++step)
    {
        client.progress(1000);
        TransferConnection(client, server);
        server.progress(1000);
    }
    NetInterfaceTlsTestPair::require(server.peek(incoming) == NI_READY && incoming.m_payload == borrowed &&
        incoming.m_requestId == 0xfedcba9876543210ULL, "unconsumed frame overwritten");
    server.consume();
    while (server.peek(incoming) != NI_READY)
    {
        netTestCheckpoint();
        client.progress(1000);
        TransferConnection(client, server);
        server.progress(1000);
    }
    NetInterfaceTlsTestPair::require(incoming.m_requestId == 42 && incoming.m_payloadSize == 3 &&
        std::memcmp(incoming.m_payload, &payload[0], 3) == 0, "leftover plaintext lost");
    server.consume();
}

void NetInterfaceConnectionTestCases::controlPriority()
{
    NetInterfaceConnection client(NetInterfaceTlsTestPair::options(false, "localhost"));
    NetInterfaceConnection server(NetInterfaceTlsTestPair::options(true, nullptr));
    HandshakeConnection(client, server);
    NetInterfaceFrame frame = {};
    frame.m_kind = NI_REQUEST;
    for (uint64_t id = 1; id <= 64; ++id)
    {
        frame.m_requestId = id;
        NetInterfaceTlsTestPair::require(client.send(frame) == NI_OK, "regular queue admission");
    }
    NetInterfaceTlsTestPair::require(client.send(frame) == NI_LIMIT_EXCEEDED, "regular count limit missing");
    frame.m_kind = NI_PING;
    for (uint64_t id = 100; id < 116; ++id)
    {
        frame.m_requestId = id;
        NetInterfaceTlsTestPair::require(client.send(frame) == NI_OK, "control capacity starved by regular traffic");
    }
    NetInterfaceTlsTestPair::require(client.send(frame) == NI_LIMIT_EXCEEDED, "control count limit missing");
    uint32_t received = 0;
    while (received < 80)
    {
        netTestCheckpoint();
        client.progress(1000);
        TransferConnection(client, server);
        NetInterfaceTlsTestPair::require(server.progress(1000) >= 0, "priority receiver failure");
        if (server.peek(frame) == NI_READY)
        {
            const uint64_t expected = received < 16 ? 100 + received : received - 15;
            NetInterfaceTlsTestPair::require(frame.m_requestId == expected &&
                frame.m_kind == (received < 16 ? NI_PING : NI_REQUEST), "control priority or frame ordering");
            server.consume();
            ++received;
        }
    }
}

void NetInterfaceConnectionTestCases::invalidPeerFrame()
{
    NetInterfaceTls attacker(NetInterfaceTlsTestPair::options(false, "localhost"));
    NetInterfaceConnection server(NetInterfaceTlsTestPair::options(true, nullptr));
    uint8_t invalid[32] = {};
    invalid[0] = 'X';
    bool sent = false;
    for (uint32_t step = 0; step < 20000; ++step)
    {
        netTestCheckpoint();
        attacker.progress(1000);
        if (attacker.status() == NET_INTERFACE_TLS_READY && !sent)
        {
            uint32_t accepted = 0;
            NetInterfaceTlsTestPair::require(attacker.write(invalid, sizeof(invalid), accepted) >= 0 &&
                accepted == sizeof(invalid), "attacker TLS write");
            sent = true;
        }
        const uint8_t* bytes = nullptr;
        uint32_t size = 0;
        uint32_t used = 0;
        attacker.peekOutput(bytes, size);
        server.feed(bytes, size, used);
        attacker.consumeOutput(used);
        const int32_t result = server.progress(1000);
        if (result < 0)
        {
            NetInterfaceTlsTestPair::require(sent && result == NI_PROTOCOL_ERROR &&
                server.progress(1001) == result && server.finishInput() == result,
                "malformed frame did not keep combined connection failed");
            return;
        }
        server.peekOutput(bytes, size);
        used = 0;
        attacker.feed(bytes, size, used);
        server.consumeOutput(used);
    }
    throw std::runtime_error("malformed peer frame was not rejected");
}

void NetInterfaceConnectionTestCases::recycledQueues()
{
    NetInterfaceConnection client(NetInterfaceTlsTestPair::options(false, "localhost"));
    NetInterfaceConnection server(NetInterfaceTlsTestPair::options(true, nullptr));
    HandshakeConnection(client, server);
    uint8_t payload[300];
    for (uint32_t round = 0; round < 64; ++round)
    {
        // 每轮填满两种预算，拒绝的第65/17项不能破坏已排队数据或后续空槽。
        for (uint32_t kind = 0; kind < 2; ++kind)
        {
            const uint32_t count = kind == 0 ? 64 : 16;
            for (uint32_t index = 0; index < count; ++index)
            {
                const uint64_t id = static_cast<uint64_t>(round) * 1000 + kind * 100 + index + 1;
                std::memset(payload, static_cast<int>(id & 255), sizeof(payload));
                NetInterfaceFrame frame = {};
                frame.m_kind = static_cast<uint16_t>(kind == 0 ? NI_REQUEST : NI_PING);
                frame.m_requestId = id;
                frame.m_payload = payload;
                frame.m_payloadSize = index % 3 == 0 ? 0 : static_cast<uint32_t>(sizeof(payload));
                NetInterfaceTlsTestPair::require(client.send(frame) == NI_OK, "queue admission after recycled slots");
                std::memset(payload, 0xef, sizeof(payload));
                if (index + 1 == count)
                {
                    NetInterfaceTlsTestPair::require(client.send(frame) == NI_LIMIT_EXCEEDED, "queue item budget lost");
                }
            }
        }
        uint32_t received = 0;
        for (uint32_t step = 0; step < 10000 && received < 80; ++step)
        {
            netTestCheckpoint();
            NetInterfaceTlsTestPair::require(client.progress(1000) >= 0, "queue sender progress");
            TransferConnection(client, server);
            NetInterfaceTlsTestPair::require(server.progress(1000) >= 0, "queue receiver progress");
            NetInterfaceFrame frame = {};
            if (server.peek(frame) != NI_READY)
            {
                continue;
            }
            const uint32_t kind = received < 16 ? 1 : 0;
            const uint32_t index = received < 16 ? received : received - 16;
            const uint64_t expected = static_cast<uint64_t>(round) * 1000 + kind * 100 + index + 1;
            NetInterfaceTlsTestPair::require(frame.m_requestId == expected && frame.m_kind == (kind == 0 ? NI_REQUEST : NI_PING) &&
                frame.m_payloadSize == (index % 3 == 0 ? 0 : sizeof(payload)), "queue identity or priority changed");
            for (uint32_t offset = 0; offset < frame.m_payloadSize; ++offset)
            {
                NetInterfaceTlsTestPair::require(frame.m_payload[offset] == static_cast<uint8_t>(expected), "queue borrowed caller storage");
            }
            NetInterfaceTlsTestPair::require(server.consume() == NI_OK, "queue consumption failed");
            ++received;
        }
        NetInterfaceTlsTestPair::require(received == 80 && client.outputState() == NI_OK, "queue failed to drain within bounded work");
        NetInterfaceFrame invalid = {};
        invalid.m_kind = 65535;
        NetInterfaceTlsTestPair::require(client.send(invalid) == NI_INVALID_ARGUMENT, "invalid frame changed queue state");
    }
    const uint32_t type = 991;
    NetInterfaceTlsTestPair::require(client.configureCompression(&type, 1, true) == NI_OK &&
        server.configureCompression(&type, 1, true) == NI_OK, "compression setup");
    std::vector<uint8_t> compressible(32768, 0x7b);
    NetInterfaceFrame compressed = {};
    compressed.m_kind = NI_REQUEST;
    compressed.m_messageType = type;
    compressed.m_requestId = 999999;
    compressed.m_payload = compressible.data();
    compressed.m_payloadSize = static_cast<uint32_t>(compressible.size());
    NetInterfaceTlsTestPair::require(client.send(compressed) == NI_OK, "compressed queue admission");
    bool complete = false;
    for (uint32_t step = 0; step < 10000 && !complete; ++step)
    {
        netTestCheckpoint();
        client.progress(1000);
        TransferConnection(client, server);
        NetInterfaceTlsTestPair::require(server.progress(1000) >= 0, "compressed queue receiver");
        NetInterfaceFrame frame = {};
        if (server.peek(frame) == NI_READY)
        {
            NetInterfaceTlsTestPair::require(frame.m_requestId == compressed.m_requestId && frame.m_payloadSize == compressible.size() &&
                std::memcmp(frame.m_payload, compressible.data(), compressible.size()) == 0, "compressed queue bytes");
            server.consume();
            complete = true;
        }
    }
    NetInterfaceTlsTestPair::require(complete && client.outputState() == NI_OK, "compressed queue retained budget");
}

void NetInterfaceConnectionTestCases::directoryBoundaries()
{
    const char* validAddresses[] = {"127.0.0.1", "1.2.3.4", "255.255.255.254", "::1", "2001:db8::1",
        "2001:0DB8:0:0:0:0:0:0001", "1:2:3:4:5:6:7:8", "1::", "::ffff:7f00:1"};
    for (size_t index = 0; index < sizeof(validAddresses) / sizeof(validAddresses[0]); ++index)
    {
        NetInterfaceTlsTestPair::require(NetInterfaceDirectory::address(validAddresses[index]), "valid numeric address rejected");
    }
    const char* invalidAddresses[] = {"", "0.0.0.0", "255.255.255.255", "127.0.0.01", "1.2.3", "1.2.3.4.",
        "1.2.3.256", "::", ":::1", "1:::2", "1::2::3", "1:2:3:4:5:6:7", "1:2:3:4:5:6:7:8::", "::1:",
        "[::1]", "fe80::1%2", "1:2:3:4:5:6:7:gggg", "::ffff:0:0", "::ffff:ffff:ffff", "localhost"};
    for (size_t index = 0; index < sizeof(invalidAddresses) / sizeof(invalidAddresses[0]); ++index)
    {
        NetInterfaceTlsTestPair::require(!NetInterfaceDirectory::address(invalidAddresses[index]), "invalid numeric address accepted");
    }
    NetInterfaceDirectoryPoint point;
    point.m_name = "node.0123456789abcdef0123456789abcdef";
    point.m_address = "2001:db8::1";
    point.m_port = 12345;
    point.m_capacity = 32;
    point.m_boot = UINT64_C(0xfedcba9876543210);
    std::vector<NetInterfaceDirectoryPoint> points(1, point);
    std::string encoded;
    NetInterfaceTlsTestPair::require(NetInterfaceDirectory::encode(points, encoded) == NI_OK, "directory encoding");
    points[0].m_name = encoded;
    NetInterfaceTlsTestPair::require(NetInterfaceDirectory::parse(reinterpret_cast<const uint8_t*>(points[0].m_name.data()),
        static_cast<uint32_t>(points[0].m_name.size()), points) == NI_OK && points.size() == 1 && points[0].m_name == point.m_name,
        "directory destroyed aliased input before parsing");
    NetInterfaceTlsTestPair::require(NetInterfaceDirectory::encode(points, points[0].m_name) == NI_OK, "encoding destroyed aliased input name");
    NetInterfaceTlsTestPair::require(NetInterfaceDirectory::parse(reinterpret_cast<const uint8_t*>(points[0].m_name.data()),
        static_cast<uint32_t>(points[0].m_name.size()), points) == NI_OK && points[0].m_boot == point.m_boot,
        "encoding alias did not preserve metadata");
    std::string cache;
    const uint64_t now = UINT64_C(10000000000);
    NetInterfaceTlsTestPair::require(NetInterfaceDirectory::encodeCache(points, now, cache) == NI_OK, "cache encoding");
    points[0].m_name = cache;
    NetInterfaceTlsTestPair::require(NetInterfaceDirectory::parseCache(reinterpret_cast<const uint8_t*>(points[0].m_name.data()),
        static_cast<uint32_t>(points[0].m_name.size()), now, points) == NI_OK && points[0].m_name == point.m_name,
        "cache destroyed aliased input before parsing");
    NetInterfaceTlsTestPair::require(NetInterfaceDirectory::parseCache(reinterpret_cast<const uint8_t*>(cache.data()), static_cast<uint32_t>(cache.size()),
        now + UINT64_C(2592000000), points) == NI_OK, "cache exact thirty-day boundary");
    NetInterfaceTlsTestPair::require(NetInterfaceDirectory::parseCache(reinterpret_cast<const uint8_t*>(cache.data()), static_cast<uint32_t>(cache.size()),
        now + UINT64_C(2592000001), points) == NI_TIMEOUT && points.empty(), "stale cache retained candidates");
    NetInterfaceTlsTestPair::require(NetInterfaceDirectory::parseCache(reinterpret_cast<const uint8_t*>(cache.data()), static_cast<uint32_t>(cache.size()),
        now - 300000, points) == NI_OK, "cache exact future allowance");
    NetInterfaceTlsTestPair::require(NetInterfaceDirectory::parseCache(reinterpret_cast<const uint8_t*>(cache.data()), static_cast<uint32_t>(cache.size()),
        now - 300001, points) == NI_TIMEOUT && points.empty(), "future cache accepted");
    points.assign(2, point);
    points[1].m_name[5] = 'f';
    points[1].m_address = "2001:0DB8:0:0:0:0:0:0001";
    NetInterfaceTlsTestPair::require(NetInterfaceDirectory::encode(points, encoded) == NI_INVALID_ARGUMENT && encoded.empty(), "equivalent IPv6 endpoints counted twice");
    points[0].m_address = "127.0.0.1";
    points[1].m_address = "::ffff:7f00:1";
    NetInterfaceTlsTestPair::require(NetInterfaceDirectory::encode(points, encoded) == NI_INVALID_ARGUMENT, "mapped IPv4 endpoint counted twice");
    points[1].m_port++;
    NetInterfaceTlsTestPair::require(NetInterfaceDirectory::encode(points, encoded) == NI_OK, "different ports incorrectly merged");
    points.assign(64, point);
    const char digits[] = "0123456789abcdef";
    for (uint32_t index = 0; index < 64; ++index)
    {
        points[index].m_name[35] = digits[index / 16];
        points[index].m_name[36] = digits[index % 16];
        points[index].m_port += index;
        points[index].m_boot += index;
    }
    NetInterfaceTlsTestPair::require(NetInterfaceDirectory::encode(points, encoded) == NI_OK && encoded.size() <= 16384, "full directory encoding");
    std::vector<NetInterfaceDirectoryPoint> parsed;
    NetInterfaceTlsTestPair::require(NetInterfaceDirectory::parse(reinterpret_cast<const uint8_t*>(encoded.data()), static_cast<uint32_t>(encoded.size()), parsed) == NI_OK &&
        parsed.size() == points.size(), "full directory parse");
    for (uint32_t index = 0; index < 64; ++index)
    {
        NetInterfaceTlsTestPair::require(parsed[index].m_name == points[index].m_name && parsed[index].m_address == points[index].m_address &&
            parsed[index].m_port == points[index].m_port && parsed[index].m_boot == points[index].m_boot, "full directory identity changed");
    }
    points.push_back(point);
    NetInterfaceTlsTestPair::require(NetInterfaceDirectory::encode(points, cache) == NI_INVALID_ARGUMENT && cache.empty(), "65th node accepted");
    points.pop_back();
    for (uint32_t size = 0; size < encoded.size(); size += static_cast<uint32_t>(encoded.size() / 64 + 1))
    {
        NetInterfaceTlsTestPair::require(NetInterfaceDirectory::parse(reinterpret_cast<const uint8_t*>(encoded.data()), size, parsed) < 0 && parsed.empty(),
            "truncated directory retained partial nodes");
    }
    NetInterfaceTlsTestPair::require(NetInterfaceDirectory::encodeCache(points, now, cache) == NI_OK, "full cache encoding");
    for (size_t offset = 0; offset < cache.size(); offset += cache.size() / 64 + 1)
    {
        cache[offset] ^= 1;
        NetInterfaceTlsTestPair::require(NetInterfaceDirectory::parseCache(reinterpret_cast<const uint8_t*>(cache.data()), static_cast<uint32_t>(cache.size()), now, parsed) < 0 && parsed.empty(),
            "tampered cache retained nodes");
        cache[offset] ^= 1;
    }
    std::vector<NetGatewayAddress> seeds;
    NetInterfaceTlsTestPair::require(NetClientDiscovery::parseSeeds("[2001:db8::1]:7,[2001:0DB8:0:0:0:0:0:0001]:7,127.0.0.1:8,[::ffff:7f00:1]:8", seeds) == NI_OK &&
        seeds.size() == 2 && seeds[0].m_address == "2001:db8::1" && seeds[1].m_address == "127.0.0.1", "seed numeric deduplication");
    seeds[0].m_address = "[2001:db8::1]:7,127.0.0.1:8";
    NetInterfaceTlsTestPair::require(NetClientDiscovery::parseSeeds(seeds[0].m_address, seeds) == NI_OK && seeds.size() == 2,
        "seed parser destroyed aliased text");
    const char* invalidSeeds[] = {",127.0.0.1:1", "127.0.0.1:1,", "127.0.0.1:1,,127.0.0.1:2", "[::1]", "[::1]x:1", "[::1]:",
        "[::1:1", "127.0.0.1:0", "127.0.0.1:65536", "127.0.0.1:123456", "127.0.0.1:-1", "127.0.0.1:1:2", "::1:2", "localhost:7"};
    for (size_t index = 0; index < sizeof(invalidSeeds) / sizeof(invalidSeeds[0]); ++index)
    {
        NetInterfaceTlsTestPair::require(NetClientDiscovery::parseSeeds(invalidSeeds[index], seeds) == NI_INVALID_ARGUMENT && seeds.empty(), "invalid seed accepted");
    }
    std::string repeated;
    for (uint32_t index = 0; index < 64; ++index)
    {
        if (index != 0) { repeated += ','; }
        repeated += "127.0.0.1:7";
    }
    NetInterfaceTlsTestPair::require(NetClientDiscovery::parseSeeds(repeated, seeds) == NI_OK && seeds.size() == 1, "64 seed limit or duplicate handling");
    repeated += ",127.0.0.1:7";
    NetInterfaceTlsTestPair::require(NetClientDiscovery::parseSeeds(repeated, seeds) == NI_INVALID_ARGUMENT && seeds.empty(), "65 seed inputs accepted after deduplication");

    {
        NetInterfaceDirectoryNodeData node = {};
        node.m_structSize = sizeof(node);
        node.m_version = 1;
        std::memcpy(node.m_name, "node.0123456789abcdef0123456789abcdef", 38);
        std::memcpy(node.m_address, "127.0.0.1", 10);
        node.m_port = 12345;
        node.m_capacity = 32;
        node.m_boot = 1;
        NetInterfaceDirectoryHandle handle = nullptr;
        NetInterfaceTlsTestPair::require(NetInterface::directoryCreateNodes(&node, 1, &handle) == NI_OK, "alias fixture creation");
        std::unique_ptr<NetInterfaceDirectoryOpaque, decltype(&NetInterface::directoryDestroy)> owner(handle, NetInterface::directoryDestroy);
        uint64_t aligned[32];
        std::memset(aligned, 0x5a, sizeof(aligned));
        const uint64_t expected = UINT64_C(0x5a5a5a5a5a5a5a5a);
        uint8_t* bytes = reinterpret_cast<uint8_t*>(aligned);
        NetInterfaceTlsTestPair::require(NetInterface::directoryCreate(bytes, sizeof(aligned), reinterpret_cast<NetInterfaceDirectoryHandle*>(aligned)) == NI_INVALID_ARGUMENT &&
            aligned[0] == expected, "create handle destroyed input");
        NetInterfaceTlsTestPair::require(NetInterface::directoryCreateCache(bytes, sizeof(aligned), 1000, 1,
            reinterpret_cast<NetInterfaceDirectoryHandle*>(aligned + 1)) == NI_INVALID_ARGUMENT &&
            aligned[1] == expected, "cache handle destroyed input");
        NetInterfaceTlsTestPair::require(NetInterface::directoryCache(handle, 1000, 1, bytes, sizeof(aligned), reinterpret_cast<uint32_t*>(aligned + 2)) == NI_INVALID_ARGUMENT &&
            aligned[2] == expected, "cache required overlaps output");
        uint32_t fields[4] = {9, 8, 7, 6};
        char* name = reinterpret_cast<char*>(bytes);
        char* address = reinterpret_cast<char*>(bytes + 64);
        NetInterfaceTlsTestPair::require(NetInterface::directoryNode(handle, 0, name, 38, name + 10, 40, fields) == NI_INVALID_ARGUMENT &&
            aligned[0] == expected && fields[0] == 9, "node overlapping strings accepted");
        NetInterfaceTlsTestPair::require(NetInterface::directoryNode(handle, 0, name, 38, address, 40, reinterpret_cast<uint32_t*>(aligned + 1)) == NI_INVALID_ARGUMENT &&
            aligned[1] == expected, "node fields overlap name");
        NetInterfaceTlsTestPair::require(NetInterface::directoryNode(handle, 0, name, 38, address, 40, reinterpret_cast<uint32_t*>(aligned + 9)) == NI_INVALID_ARGUMENT &&
            aligned[9] == expected, "node fields overlap address");
        NetInterfaceTlsTestPair::require(NetInterface::directoryNode(handle, 0, name, 38, address, 40, fields) == NI_OK &&
            fields[0] == 12345 && std::strcmp(address, "127.0.0.1") == 0, "alias failure corrupted handle");
    }

}

void NetInterfaceConnectionTestCases::gatewayChannelLifecycle()
{
    NetInterfaceTlsOptions options = {};
    options.handshakeTimeoutMs = 5000;
    options.entropy = NetInterfaceTlsTestPair::strongRandom;
    for (uint32_t mode = 0; mode < 4; ++mode)
    {
        NetGatewayChannelTestTransport transport;
        NetGatewayChannelTestListener listener;
        NetGatewayClientChannel channel(101, &transport);
        listener.m_channel = &channel;
        listener.m_throw = true;
        transport.m_stopThrow = true;
        transport.m_closeDuringStop = true;
        transport.m_pollThrow = mode == 0;
        transport.m_connectResult = mode == 2 ? NI_NOT_FOUND : NI_OK;
        transport.m_zeroPeer = mode == 3;
        uint64_t peer = 99;
        const int32_t connected = channel.connect("127.0.0.1", 1, options, &listener, peer);
        if (mode >= 2)
        {
            NetInterfaceTlsTestPair::require(connected == (mode == 2 ? NI_NOT_FOUND : NI_PROTOCOL_ERROR) && peer == 0 &&
                listener.m_closed == 0 && transport.m_stops == 1 && transport.m_listener == nullptr, "unaccepted channel cleanup/false callback");
        }
        else
        {
            NetInterfaceTlsTestPair::require(connected == NI_OK && peer == 1, "channel connect");
            if (mode == 1) { channel.close(1, NI_CANCELLED); }
            NetInterfaceTlsTestPair::require(!channel.poll() && listener.m_closed == 1 &&
                listener.m_reason == (mode == 0 ? NI_NO_MEMORY : NI_CANCELLED), "double exception lost close or first reason");
        }
        for (uint32_t index = 0; index < 1000; ++index)
        {
            netTestCheckpoint();
            NetInterfaceTlsTestPair::require(!channel.poll(), "closed channel still polling");
        }
        channel.shutdown();
        NetInterfaceTlsTestPair::require(transport.m_stops == 1 && listener.m_closed == (mode < 2 ? 1U : 0U) && listener.m_other == 0,
            "duplicate callback during reentrant stop");
    }
    NetGatewayChannelTestTransport transport;
    NetGatewayChannelTestListener listener;
    NetGatewayClientChannel channel(101, &transport);
    transport.m_busy = true;
    uint64_t peer = 0;
    NetInterfaceTlsTestPair::require(channel.connect("127.0.0.1", 1, options, &listener, peer) == NI_OK && channel.poll(), "busy channel setup");
    uint8_t expected[256];
    const uint32_t size = transport.m_controlSize;
    std::memcpy(expected, transport.m_control, size);
    for (uint32_t index = 0; index < 4096; ++index)
    {
        netTestCheckpoint();
        NetInterfaceTlsTestPair::require(channel.poll() && transport.m_controlSize == size &&
            std::memcmp(expected, transport.m_control, size) == 0, "busy OPEN mutated or advanced phase");
    }
    transport.m_busy = false;
    NetInterfaceTlsTestPair::require(channel.poll() && transport.m_controls == 4098 && channel.poll() && transport.m_controls == 4098,
        "accepted OPEN replayed");
    channel.shutdown();
    NetInterfaceTlsTestPair::require(listener.m_closed == 1 && listener.m_reason == NI_OK, "busy channel normal close");
}

void NetInterfaceConnectionTestCases::recoveryLifecycle()
{
    NetInterfaceTlsOptions tls = {};
    tls.handshakeTimeoutMs = 1000;
    tls.entropy = NetClientSessionTestListener::entropy;
    NetInterfaceHandshakeOptions hello = {};
    hello.m_platform = 1;
    hello.m_capabilities = 7;
    hello.m_maxRequests = 4;
    hello.m_maxChunk = 4096;
    hello.m_timeoutMs = 1000;
    std::memset(hello.m_device, 0x31, sizeof(hello.m_device));
#define RECOVERY_CHECK(condition) NetInterfaceTlsTestPair::require((condition), #condition)

    const uint32_t startFaults[] = { 1, 3, 32, 34 };
    for (uint32_t index = 0; index < 4; ++index)
    {
        NetClientSessionTestCounters c = {};
        NetClientSessionTestFactory f(c);
        NetClientSessionTestDiscovery d;
        NetClientSessionTestListener l;
        NetClientSession s;
        d.m_fault = startFaults[index];
        d.m_drain = true;
        RECOVERY_CHECK(s.bindGateway(&f, &d) == NI_OK);
        RECOVERY_CHECK(s.connectService(101, "127.0.0.1", 1, tls, hello, &l) ==
            (index < 2 ? NI_NO_MEMORY : NI_INVALID_ARGUMENT));
        RECOVERY_CHECK(s.generation() == 0 && l.m_generations.empty() && c.m_created == 0);
        RECOVERY_CHECK(!s.quiescent() && s.bindGateway(nullptr) == NI_INVALID_STATE);
        RECOVERY_CHECK(s.poll() && !s.poll() && s.quiescent());
        RECOVERY_CHECK(s.bindGateway(nullptr) == NI_OK && l.m_generations.empty());
    }
    const uint32_t activeFaults[] = { 4, 8, 2, 16 };
    for (uint32_t index = 0; index < 4; ++index)
    {
        NetClientSessionTestCounters c = {};
        NetClientSessionTestFactory f(c);
        NetClientSessionTestDiscovery d;
        NetClientSessionTestListener l;
        NetClientSession s;
        d.m_drain = true;
        l.m_nestedSession = &s;
        l.m_reenterClosed = true;
        l.m_throwClosed = index == 1;
        RECOVERY_CHECK(s.bindGateway(&f, &d) == NI_OK);
        RECOVERY_CHECK(s.connectService(101, "127.0.0.1", 1, tls, hello, &l) == NI_OK);
        d.m_fault = activeFaults[index];
        if (index >= 2)
        {
            s.close(NI_CANCELLED);
            s.close(NI_TIMEOUT);
        }
        RECOVERY_CHECK(s.poll() && l.m_generations.size() == 1 && l.m_generations[0] == 1 &&
            l.m_reasons[0] == (index < 2 ? NI_NO_MEMORY : NI_CANCELLED));
        for (uint32_t loop = 0; loop < 8; ++loop)
        {
            s.poll();
            RECOVERY_CHECK(!s.executing() && l.m_generations.size() == 1);
        }
        RECOVERY_CHECK(l.m_nestedExecuting && l.m_nestedCancelOk && c.m_created == 0);
        if (index == 3)
        {
            RECOVERY_CHECK(!s.quiescent() && s.bindGateway(nullptr) == NI_INVALID_STATE);
            d.m_fault = 0;
        }
        RECOVERY_CHECK(!s.poll() && s.quiescent());
        s.shutdown();
        RECOVERY_CHECK(l.m_generations.size() == 1 && l.m_other == 0 && s.bindGateway(nullptr) == NI_OK);
    }
    NetClientSessionTestCounters fixedCounters = {};
    NetClientSessionTestFactory fixedFactory(fixedCounters);
    NetClientSessionTestDiscovery unusedDiscovery;
    unusedDiscovery.m_fault = 1;
    NetClientSessionTestListener fixedListener;
    NetClientSession fixedSession;
    RECOVERY_CHECK(fixedSession.bindGateway(&fixedFactory, &unusedDiscovery) == NI_OK);
    RECOVERY_CHECK(fixedSession.configureGateways("127.0.0.1:1234", "") == NI_OK);
    RECOVERY_CHECK(fixedSession.connectService(101, "local-ipc", 1, tls, hello, &fixedListener, &fixedFactory) == NI_OK);
    NetInterfaceFrame wrongProduct = {};
    wrongProduct.m_productId = 102;
    RECOVERY_CHECK(fixedSession.sendData(wrongProduct) == NI_INVALID_ARGUMENT && unusedDiscovery.m_fault == 1);
    fixedSession.close(NI_CANCELLED);
    RECOVERY_CHECK(!fixedSession.poll() && fixedListener.m_generations.size() == 1);
#undef RECOVERY_CHECK
}

void NetInterfaceConnectionTestCases::recoveryDeadline()
{
    NetInterfaceTlsOptions tls = {};
    tls.handshakeTimeoutMs = 1000;
    tls.entropy = NetClientSessionTestListener::entropy;
    NetInterfaceHandshakeOptions hello = {};
    hello.m_platform = 1;
    hello.m_capabilities = 7;
    hello.m_maxRequests = 4;
    hello.m_maxChunk = 4096;
    hello.m_timeoutMs = 1000;
    std::memset(hello.m_device, 0x31, sizeof(hello.m_device));
    NetClientSessionTestCounters counters = {};
    NetClientSessionTestFactory factory(counters);
    NetClientSessionTestListener listener;
    NetClientSession session;
    NetInterfaceTlsTestPair::require(session.bindGateway(&factory) == NI_OK &&
        session.connectService(101, "127.0.0.1", 1, tls, hello, &listener) == NI_OK, "deadline start");
    const uint64_t deadline = NetInterfaceConnectionTestCasesHelper::recoveryTestNow() + 30100;
    while (NetInterfaceConnectionTestCasesHelper::recoveryTestNow() < deadline)
    {
        netTestCheckpoint();
        CSystem::Sleep(10);
    }
    NetInterfaceTlsTestPair::require(!session.poll() && counters.m_created == 0 && listener.m_generations.size() == 1 &&
        listener.m_reasons[0] == NI_TIMEOUT && session.quiescent(), "expired recovery created new transport or lost timeout");
    session.shutdown();
    NetInterfaceTlsTestPair::require(listener.m_generations.size() == 1, "deadline duplicate close");
}

void NetInterfaceConnectionTestCases::protocolSessionLifecycle()
{
#define P_CHECK(condition) NetInterfaceTlsTestPair::require((condition), #condition)
    NetInterfaceTlsOptions tls = {};
    tls.handshakeTimeoutMs = 1000;
    tls.entropy = NetClientSessionTestListener::entropy;
    NetInterfaceHandshakeOptions hello = {};
    hello.m_platform = 1;
    hello.m_capabilities = 7;
    hello.m_maxRequests = 128;
    hello.m_maxChunk = 4096;
    hello.m_timeoutMs = 1000;
    std::memset(hello.m_device, 0x31, sizeof(hello.m_device));
    NetInterfaceHandshakeOptions server = hello;
    server.m_server = 1;
    server.m_platform = 0;
    std::memset(server.m_device, 0, sizeof(server.m_device));
    std::memset(server.m_nonce, 0x73, sizeof(server.m_nonce));
    for (uint32_t mode = 0; mode < 8; ++mode)
    {
        NetClientSessionTestChannel channel(server);
        NetClientSessionTestListener listener;
        NetSession session;
        P_CHECK(session.bindChannel(&channel) == NI_OK && session.connect("mock", 1, tls, hello, &listener) == NI_OK);
        for (uint32_t index = 0; index < 8 && !session.ready(); ++index)
        {
            session.poll();
        }
        P_CHECK(session.ready() && listener.m_other == 1);
        uint64_t ids[3] = {};
        for (uint32_t index = 0; index < 3; ++index)
        {
            P_CHECK(session.beginRequest(101, 11 + index, 1000, ids[index]) == NI_OK && ids[index] == index + 1);
        }
        channel.m_fault = mode == 0 ? 1 : mode == 1 ? 2 : mode == 2 ? 4 : mode == 3 ? 16 :
            mode == 4 ? 32 : mode == 5 ? 64 : mode == 6 ? 1024 : 0;
        if (mode == 0)
        {
            const uint8_t body[] = { 0x71, 0x72, 0x73 };
            P_CHECK(session.sendRequest(ids[0], body, sizeof(body)) == NI_INVALID_STATE);
            P_CHECK(session.cancel(ids[0]) == NI_OK);
        }
        else if (mode == 1)
        {
            P_CHECK(session.sendRequest(ids[0], nullptr, 0) == NI_OK);
            P_CHECK(session.cancel(ids[0]) == NI_OK && !session.ready());
            P_CHECK(listener.m_resultIds.size() == 1 && listener.m_resultStatus[0] == NI_RPC_CANCELLED);
        }
        else if (mode == 2)
        {
            NetInterfaceFrame frame = {};
            frame.m_kind = NI_DATA;
            frame.m_productId = 101;
            frame.m_requestId = 1;
            P_CHECK(session.sendData(frame) == NI_INVALID_STATE);
        }
        else if (mode == 3)
        {
            P_CHECK(session.outputState() == NI_INVALID_STATE);
        }
        else if (mode == 4)
        {
            session.close(NI_CANCELLED);
            P_CHECK(!session.ready() && listener.m_generations.size() == 1);
        }
        else if (mode == 5)
        {
            session.poll();
            P_CHECK(!session.ready());
        }
        else if (mode == 6)
        {
            for (uint32_t index = 0; index < 128; ++index)
            {
                P_CHECK(session.sendRequest(ids[0], nullptr, 0) == NI_LIMIT_EXCEEDED && listener.m_resultIds.empty());
            }
            channel.m_fault = 0;
            P_CHECK(session.sendRequest(ids[0], nullptr, 0) == NI_OK);
            P_CHECK(session.sendRequest(ids[0], nullptr, 0) == NI_INVALID_STATE);
            P_CHECK(session.cancel(ids[0]) == NI_OK);
        }
        else
        {
            listener.m_throwResult = true;
            P_CHECK(session.cancel(ids[0]) == NI_OK && !session.ready());
        }
        session.shutdown();
        P_CHECK(listener.m_resultIds.size() == 3 && listener.m_resultStatus.size() == 3 &&
            listener.m_generations.size() == 1 && listener.m_events.size() == 4 && listener.m_events.back() == UINT64_MAX);
        std::sort(listener.m_resultIds.begin(), listener.m_resultIds.end());
        P_CHECK(listener.m_resultIds[0] == 1 && listener.m_resultIds[1] == 2 && listener.m_resultIds[2] == 3);
        for (uint32_t index = 0; index < 3; ++index)
        {
            P_CHECK(session.cancel(ids[index]) == NI_NOT_FOUND);
        }
        P_CHECK(!session.poll());
        session.shutdown();
        P_CHECK(listener.m_generations.size() == 1 && listener.m_events.size() == 4);
    }
    const uint32_t connectFaults[] = { 128, 256, 512 };
    for (uint32_t index = 0; index < 3; ++index)
    {
        NetClientSessionTestChannel channel(server);
        NetClientSessionTestListener listener;
        NetSession session;
        channel.m_fault = connectFaults[index];
        P_CHECK(session.bindChannel(&channel) == NI_OK);
        P_CHECK(session.connect("mock", 1, tls, hello, &listener) ==
            (index == 0 ? NI_PROTOCOL_ERROR : index == 1 ? NI_NO_MEMORY : NI_NOT_FOUND));
        session.shutdown();
        P_CHECK(!session.poll() && listener.m_generations.empty() && listener.m_resultIds.empty() && listener.m_other == 0);
    }
    for (uint32_t mode = 0; mode < 4; ++mode)
    {
        NetClientSessionTestChannel channel(server);
        NetClientSessionTestListener listener;
        NetSession session;
        P_CHECK(session.bindChannel(&channel) == NI_OK && session.connect("mock", 1, tls, hello, &listener) == NI_OK);
        for (uint32_t index = 0; index < 8 && !session.ready(); ++index)
        {
            session.poll();
        }
        P_CHECK(session.ready());
        uint64_t id = 0;
        P_CHECK(session.beginRequest(101, 11, 1000, id) == NI_OK);
        if (mode != 3)
        {
            P_CHECK(session.sendRequest(id, nullptr, 0) == NI_OK);
        }
        uint8_t payload[18] = { 'N', 'R', 'P', '2' };
        payload[16] = 0x5a;
        payload[17] = 0xa5;
        NetInterfaceFrame frame = {};
        frame.m_kind = NI_RESPONSE;
        frame.m_productId = mode == 1 ? 102 : 101;
        frame.m_messageType = mode == 2 ? 12 : 11;
        frame.m_requestId = id;
        frame.m_payload = payload;
        frame.m_payloadSize = sizeof(payload);
        channel.queueFrame(frame);
        session.poll();
        if (mode == 0)
        {
            P_CHECK(listener.m_resultIds.size() == 1 && listener.m_resultStatus[0] == NI_RPC_SUCCESS && session.ready());
            channel.queueFrame(frame);
            session.poll();
            P_CHECK(listener.m_resultIds.size() == 1 && session.ready());
        }
        session.shutdown();
        P_CHECK(listener.m_resultIds.size() == 1 && listener.m_resultIds[0] == id &&
            listener.m_resultStatus[0] == (mode == 0 ? NI_RPC_SUCCESS : NI_RPC_UNAVAILABLE));
        P_CHECK(listener.m_generations.size() == 1 && listener.m_events.back() == UINT64_MAX);
    }
    {
        NetClientSessionTestChannel channel(server);
        NetClientSessionTestListener listener;
        NetSession session;
        P_CHECK(session.bindChannel(&channel) == NI_OK && session.connect("mock", 1, tls, hello, &listener) == NI_OK);
        for (uint32_t index = 0; index < 8 && !session.ready(); ++index)
        {
            session.poll();
        }
        for (uint32_t index = 0; index < 128; ++index)
        {
            uint64_t id = 0;
            P_CHECK(session.beginRequest(101, 11, 60000, id) == NI_OK && id == index + 1);
        }
        uint64_t id = 99;
        P_CHECK(session.beginRequest(101, 11, 60000, id) == NI_LIMIT_EXCEEDED && id == 0);
        P_CHECK(session.cancel(1) == NI_OK);
        P_CHECK(session.beginRequestAt(129, 101, 11, NetInterfaceConnectionTestCasesHelper::recoveryTestNow(), 60000) == NI_PROTOCOL_ERROR);
        P_CHECK(session.beginRequest(101, 11, 60000, id) == NI_OK && id == 130);
        session.shutdown();
        P_CHECK(listener.m_resultIds.size() == 129 && listener.m_generations.size() == 1);
        for (uint32_t index = 0; index < listener.m_resultStatus.size(); ++index)
        {
            P_CHECK(listener.m_resultStatus[index] == NI_RPC_CANCELLED);
        }
    }

    for (uint32_t mode = 0; mode < 2; ++mode)
    {
        NetClientSessionTestChannel channel(server);
        NetClientSessionTestListener listener;
        NetSession session;
        P_CHECK(session.bindChannel(&channel) == NI_OK && session.connect("mock", 1, tls, hello, &listener) == NI_OK);
        for (uint32_t index = 0; index < 8 && !session.ready(); ++index)
        {
            session.poll();
        }
        uint64_t id = 0;
        P_CHECK(session.beginRequest(101, 11, 10, id) == NI_OK && session.sendRequest(id, nullptr, 0) == NI_OK);
        uint8_t payload[16] = { 'N', 'R', 'P', '2' };
        NetInterfaceFrame response = {};
        response.m_kind = NI_RESPONSE;
        response.m_productId = 101;
        response.m_messageType = 11;
        response.m_requestId = id;
        response.m_payload = payload;
        response.m_payloadSize = sizeof(payload);
        if (mode == 0)
        {
            channel.queueFrame(response);
        }
        else
        {
            channel.m_fault = 2;
        }
        const uint64_t afterDeadline = NetInterfaceConnectionTestCasesHelper::recoveryTestNow() + 11;
        while (NetInterfaceConnectionTestCasesHelper::recoveryTestNow() < afterDeadline)
        {
            // 测试固定11毫秒，不创建生产线程，不替代真实30秒恢复期限用例。
        }
        session.poll();
        P_CHECK(listener.m_resultIds.size() == 1 && listener.m_resultIds[0] == id &&
            listener.m_resultStatus[0] == NI_RPC_TIMEOUT && session.ready() == (mode == 0));
        session.shutdown();
        P_CHECK(listener.m_events.size() == 2 && listener.m_events.back() == UINT64_MAX);
    }

    {
        NetInterfaceHandshakeOptions compressedHello = hello;
        NetInterfaceHandshakeOptions compressedServer = server;
        compressedHello.m_capabilities = compressedServer.m_capabilities = 15;
        NetClientSessionTestChannel channel(compressedServer);
        NetClientSessionTestListener listener;
        NetSession session;
        channel.m_fault = 8;
        P_CHECK(session.bindChannel(&channel) == NI_OK && session.connect("mock", 1, tls, compressedHello, &listener) == NI_OK);
        for (uint32_t index = 0; index < 8; ++index)
        {
            session.poll();
        }
        session.shutdown();
        P_CHECK(!session.ready() && listener.m_generations.size() == 1 && listener.m_other == 0);
    }
    {
        NetClientSessionTestChannel channel(server);
        NetClientSessionTestListener listener;
        NetSession session;
        P_CHECK(session.bindChannel(&channel) == NI_OK && session.connect("mock", 1, tls, hello, &listener) == NI_OK);
        for (uint32_t index = 0; index < 8 && !session.ready(); ++index)
        {
            session.poll();
        }
        const uint32_t count = channel.m_sendCount;
        NetInterfaceFrame frame = {};
        frame.m_kind = NI_DATA;
        frame.m_productId = 101;
        frame.m_requestId = 1;
        frame.m_payloadSize = 1;
        P_CHECK(session.sendData(frame) == NI_INVALID_ARGUMENT && channel.m_sendCount == count);
        session.shutdown();
    }
#undef P_CHECK
}

void NetInterfaceConnectionTestCases::partialFrameControl()
{
    {
        NetInterfaceConnection client(NetInterfaceTlsTestPair::options(false, "localhost"));
        NetInterfaceConnection server(NetInterfaceTlsTestPair::options(true, nullptr));
        HandshakeConnection(client, server);
        std::vector<uint8_t> payload(1048576);
        for (size_t index = 0; index < payload.size(); ++index)
        {
            payload[index] = static_cast<uint8_t>(index * 37 + 11);
        }
        NetInterfaceFrame outgoing = {};
        outgoing.m_kind = NI_REQUEST;
        outgoing.m_requestId = 1;
        outgoing.m_payload = payload.data();
        outgoing.m_payloadSize = static_cast<uint32_t>(payload.size());
        NetInterfaceTlsTestPair::require(client.send(outgoing) == NI_OK, "large frame admission");
        NetInterfaceTlsTestPair::require(client.progress(1000) >= 0 && client.outputState() == NI_READY, "partial large frame starts");
        const uint8_t* encrypted = nullptr;
        uint32_t encryptedSize = 0;
        NetInterfaceTlsTestPair::require(client.peekOutput(encrypted, encryptedSize) >= 0 && encrypted != nullptr &&
            encryptedSize > 0 && encryptedSize <= 32768, "bounded TLS output held");
        // 大帧已经交出前缀，后到控制帧只能等其完整结束，不能插进明文中间。
        outgoing.m_payload = nullptr;
        outgoing.m_payloadSize = 0;
        outgoing.m_kind = NI_PING;
        for (uint64_t id = 100; id < 116; ++id)
        {
            outgoing.m_requestId = id;
            NetInterfaceTlsTestPair::require(client.send(outgoing) == NI_OK, "control reserve during partial frame");
        }
        NetInterfaceTlsTestPair::require(client.send(outgoing) == NI_LIMIT_EXCEEDED, "full control reserve rejects transactionally");
        outgoing.m_kind = NI_REQUEST;
        outgoing.m_requestId = 2;
        NetInterfaceTlsTestPair::require(client.send(outgoing) == NI_OK, "ordinary successor admitted");
        uint32_t received = 0;
        for (uint32_t step = 0; step < 20000 && received < 18; ++step)
        {
            NetInterfaceTlsTestPair::require(client.progress(1000) >= 0, "partial sender progress");
            TransferConnection(client, server);
            NetInterfaceTlsTestPair::require(server.progress(1000) >= 0, "partial receiver progress");
            NetInterfaceFrame incoming = {};
            if (server.peek(incoming) != NI_READY)
            {
                continue;
            }
            const uint64_t expectedId = received == 0 ? 1 : received == 17 ? 2 : 99 + received;
            const uint16_t expectedKind = static_cast<uint16_t>(received == 0 || received == 17 ? NI_REQUEST : NI_PING);
            NetInterfaceTlsTestPair::require(incoming.m_requestId == expectedId && incoming.m_kind == expectedKind, "active frame/control/successor ordering");
            if (received == 0)
            {
                NetInterfaceTlsTestPair::require(incoming.m_payloadSize == payload.size() && std::memcmp(incoming.m_payload, payload.data(), payload.size()) == 0,
                    "partial payload preserved through control insertion");
                const uint8_t* borrowed = incoming.m_payload;
                // 接收者持续占用首帧，后到控制帧不得覆盖已借用数据。
                for (uint32_t hold = 0; hold < 100; ++hold)
                {
                    NetInterfaceTlsTestPair::require(client.progress(1000) >= 0, "held sender progress");
                    TransferConnection(client, server);
                    NetInterfaceTlsTestPair::require(server.progress(1000) >= 0 && server.peek(incoming) == NI_READY && incoming.m_payload == borrowed &&
                        incoming.m_requestId == 1 && std::memcmp(incoming.m_payload, payload.data(), payload.size()) == 0,
                        "borrowed large frame remains stable under control backlog");
                }
            }
            else
            {
                NetInterfaceTlsTestPair::require(incoming.m_payloadSize == 0, "control and successor payload boundary");
            }
            NetInterfaceTlsTestPair::require(server.consume() == NI_OK, "frame consumed once");
            ++received;
        }
        NetInterfaceTlsTestPair::require(received == 18, "bounded delivery of all accepted frames");
        for (uint32_t step = 0; step < 16; ++step)
        {
            NetInterfaceTlsTestPair::require(client.progress(1000) >= 0, "final sender progress");
            TransferConnection(client, server);
            NetInterfaceTlsTestPair::require(server.progress(1000) >= 0, "final receiver progress");
        }
        NetInterfaceFrame extra = {};
        NetInterfaceTlsTestPair::require(server.peek(extra) == NI_NEED_MORE && client.outputState() == NI_OK, "no duplicate and all budgets drained");
    }
    netTestCheckpoint();
}