#include "NetDiscoveryTableTest.h"
#include <stdexcept>
#include <algorithm>
#include "NetPeer/NetPeerAPI.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <cstdio>
#include <cstring>
#include <stdexcept>
#if defined(STATIC_LIB)
#include "NetPeer/NetGatewayDiscoveryState.h"
#endif
#if defined(_MSC_VER) && defined(_DEBUG)
#include <crtdbg.h>
#endif

int NetDiscoveryTableTest::check()
{
    uint32_t checks = 0;
    const auto Check = [&checks](bool passed, const char* message)
    {
        ++checks;
        if (!passed) { throw std::runtime_error(message); }
    };
    try
    {
        NetClientDiscoveryPoint output = {};
        output.m_structSize = sizeof(output);
        output.m_version = 1;
        uint32_t count = 17;
        Check(NetClientDiscovery::parseSeeds("[2001:db8::1]:7,[2001:0DB8:0:0:0:0:0:0001]:7", &output, 1, count) == NI_OK && count == 1,
            "canonical seed parser");
        uint8_t key[16] = {};
        const uint8_t expected[16] = {0x20, 1, 0x0d, 0xb8, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
        Check(NetClientDiscovery::addressKey(output, key) && std::memcmp(key, expected, 16) == 0, "independent IPv6 key");
        Check(NetClientDiscovery::parseSeeds("127.0.0.1:12345", &output, 1, count) == NI_OK, "IPv4 key fixture");
        const uint8_t mapped[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xff, 0xff, 127, 0, 0, 1};
        Check(NetClientDiscovery::addressKey(output, key) && std::memcmp(key, mapped, 16) == 0, "independent IPv4 mapped key");
        const NetClientDiscoveryPoint kept = output;
        Check(NetClientDiscovery::parseSeeds("127.0.0.2:9", &output, 1, output.m_port) == NI_INVALID_ARGUMENT &&
            std::memcmp(&kept, &output, sizeof(output)) == 0, "count overlaps output before writes");
        std::string text = "127.0.0.1:9";
        count = 42;
        Check(NetClientDiscovery::parseSeeds(text, reinterpret_cast<NetClientDiscoveryPoint*>(&text), 1, count) == NI_INVALID_ARGUMENT &&
            text == "127.0.0.1:9" && count == 42, "output overlaps string object");
        output.m_port = 0;
        Check(!NetClientDiscovery::addressKey(output, key) && std::memcmp(key, mapped, 16) == 0, "invalid key changed output");
        output = kept;
        Check(!NetClientDiscovery::addressKey(output, nullptr), "null key");
        std::memset(output.m_address, '1', sizeof(output.m_address));
        Check(!NetClientDiscovery::addressKey(output, key), "unbounded address");
        output = kept;

#if defined(STATIC_LIB)
        {
            NetGatewayDiscoveryState table;
            table.m_seedCount = table.m_nodeCount = 64;
            table.m_seeds.resize(64);
            table.m_nodes.resize(64);
            for (uint32_t index = 0; index < 64; ++index)
            {
                table.m_seeds[index] = kept;
                table.m_seeds[index].m_port = 10000 + index;
                table.m_nodes[index] = NetInterfaceDirectoryNodeData();
                std::strcpy(table.m_nodes[index].m_address, "::ffff:7f00:1");
                table.m_nodes[index].m_port = 20000 + index;
            }
            table.rebuild();
            Check(table.m_candidateCount == 128, "full candidate bound");
            for (uint32_t index = 0; index < 128; ++index)
            {
                Check(table.m_candidates[index].m_point.m_port == (index < 64 ? 10000 + index : 20000 + index - 64) &&
                    table.m_candidates[index].m_failedUntil == 0, "full candidate ordering");
                table.m_candidates[index].m_failedUntil = index == 63 ? UINT64_MAX : 5000 + index;
            }
            for (uint32_t index = 0; index < 64; ++index)
            {
                table.m_nodes[index].m_port = 10032 + index;
            }
            table.m_next = UINT32_MAX;
            table.rebuild();
            Check(table.m_candidateCount == 96 && table.m_next < 96, "mapped dedup/cursor bound");
            for (uint32_t index = 0; index < 96; ++index)
            {
                Check(table.m_candidates[index].m_point.m_port == 10000 + index, "merge dropped seed or new node");
                Check(table.m_candidates[index].m_failedUntil == (index == 63 ? UINT64_MAX : index < 64 ? 5000 + index : 0),
                    "numeric key did not retain failure deadline");
            }
            table.m_nodeCount = 0;
            table.m_nodes.clear();
            table.rebuild();
            Check(table.m_candidateCount == 64, "empty directory removed seeds");
            table.m_nodeCount = 1;
            table.m_nodes.resize(1);
            std::strcpy(table.m_nodes[0].m_address, "127.0.0.1");
            table.m_nodes[0].m_port = 20000;
            table.rebuild();
            Check(table.m_candidateCount == 65 && table.m_candidates[64].m_failedUntil == 0, "removed node retained stale failure");
        }
        std::vector<NetGatewayAddress> alias(1);
        alias[0].m_address = "127.0.0.1:12345,[2001:db8::1]:23456";
        Check(NetGatewayDiscovery::parseSeeds(alias[0].m_address, alias) == NI_OK && alias.size() == 2 &&
            alias[0].m_address == "127.0.0.1" && alias[1].m_port == 23456, "aliased wrapper input");
        Check(NetGatewayDiscovery::parseSeeds("bad", alias) == NI_INVALID_ARGUMENT && alias.empty(), "invalid wrapper clears output");
#endif
        {
            std::string configuration;
            for (uint32_t index = 0; index < 1024; ++index)
            {
                char address[64];
                std::sprintf(address, "%s127.0.0.1:%u", index == 0 ? "" : ",", 10000 + index);
                configuration += address;
            }
            std::vector<NetGatewayAddress> many;
            Check(NetGatewayDiscovery::parseSeeds(configuration, many) == NI_OK && many.size() == 1024, "seed parser has no 64-record gate");
            NetGatewayDiscovery growing;
            Check(growing.start(many, "") == NI_OK, "start 1024 candidates");
            for (uint32_t index = 0; index < 2048; ++index)
            {
                Check(growing.choose(output) && output.m_port == 10000 + index % 1024, "large candidate rotation");
            }
            growing.shutdown();
        }
        NetInterfaceTlsOptions tls = {};
        uint32_t entropyCalls = 0;
        tls.entropyContext = &entropyCalls;
        tls.entropy = [](void* context, uint8_t* bytes, uint32_t size) -> int32_t
        {
            ++*static_cast<uint32_t*>(context);
            std::memset(bytes, 0, size);
            return NI_OK;
        };
        std::vector<NetGatewayAddress> seeds(4);
        seeds[0].m_address = "2001:0db8:0000:0000:0000:0000:0000:0001";
        seeds[0].m_port = 12345;
        seeds[1].m_address = "2001:db8::1";
        seeds[1].m_port = 12345;
        seeds[2].m_address = "127.0.0.1";
        seeds[2].m_port = 23456;
        seeds[3].m_address = "::ffff:7f00:1";
        seeds[3].m_port = 23456;
        NetGatewayDiscovery discovery;
        Check(discovery.stopped() && !discovery.poll() && !discovery.choose(output), "new discovery resources");
        Check(discovery.start(seeds, "", tls) == NI_OK && !discovery.stopped() && discovery.discovered() == 0, "start seed hints");
        Check(discovery.start(seeds, "", tls) < 0, "second start accepted");
        for (uint32_t index = 0; index < 1000; ++index)
        {
            Check(discovery.choose(output) && output.m_port == (index % 2 == 0 ? 12345 : 23456), "canonical rotation");
            Check(std::strcmp(output.m_address, seeds[index % 2 == 0 ? 0 : 2].m_address.c_str()) == 0, "first seed spelling");
        }
        std::strcpy(output.m_address, "2001:DB8::1");
        output.m_port = 12345;
        discovery.failed(output);
        Check(entropyCalls == 1, "failure not found by key");
        Check(discovery.choose(output) && output.m_port == 23456, "failed duplicate selected");
        output.m_port = 30000;
        discovery.failed(output);
        Check(entropyCalls == 1, "unknown endpoint consumes entropy");
        discovery.shutdown();
        discovery.shutdown();
        Check(discovery.stopped() && !discovery.poll() && !discovery.choose(output) && discovery.discovered() == 0, "stop not idempotent");

        tls.entropy = [](void* context, uint8_t*, uint32_t) -> int32_t
        {
            ++*static_cast<uint32_t*>(context);
            throw 7;
        };
        for (uint32_t cycle = 0; cycle < 100; ++cycle)
        {
            Check(discovery.start(seeds, "", tls) == NI_OK, "restart");
            for (uint32_t index = 0; index < 2; ++index)
            {
                Check(discovery.choose(output), "restart did not clear backoff");
                discovery.failed(output);
            }
            Check(!discovery.choose(output), "entropy exception failed open");
            discovery.shutdown();
            Check(discovery.stopped(), "restart resource leak");
        }

        seeds.resize(64);
        for (uint32_t index = 0; index < 64; ++index)
        {
            seeds[index].m_address = "2001:0db8:0000:0000:0000:0000:0000:0001";
            seeds[index].m_port = 10000 + index;
        }
        Check(discovery.start(seeds, "", tls) == NI_OK, "64 seeds rejected");
#if defined(_MSC_VER) && defined(_DEBUG)
        static uint32_t s_allocations = 0;
        const auto Hook = [](int operation, void*, size_t, int block, long, const unsigned char*, int) -> int
        {
            if (operation == _HOOK_ALLOC && block == _NORMAL_BLOCK) { ++s_allocations; }
            return 1;
        };
        _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(Hook);
        bool ordered = true;
        for (uint32_t index = 0; index < 64000; ++index)
        {
            ordered = discovery.choose(output) && output.m_port == 10000 + index % 64 && ordered;
        }
        _CrtSetAllocHook(previous);
        Check(ordered && s_allocations == 0, "choose allocated or broke rotation");
#else
        for (uint32_t index = 0; index < 64000; ++index)
        {
            Check(discovery.choose(output) && output.m_port == 10000 + index % 64, "64 candidate rotation");
        }
#endif
        for (uint32_t index = 0; index < 64; ++index)
        {
            Check(discovery.choose(output) && output.m_port == 10000 + index, "failure distribution");
            discovery.failed(output);
        }
        Check(!discovery.choose(output), "blocked table yields candidate");
        discovery.shutdown();
        seeds.push_back(seeds[0]);
        Check(discovery.start(seeds, "", tls) == NI_OK, "seed storage grows and deduplicates beyond 64");
    discovery.shutdown();
        seeds.resize(1);
        const char* invalid[] = {"", "0.0.0.0", "::", "255.255.255.255", "localhost", "[::1]", "::1%lo", "1.2.3.4x"};
        for (uint32_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]); ++index)
        {
            seeds[0].m_address = invalid[index];
            Check(discovery.start(seeds, "", tls) == NI_INVALID_ARGUMENT && discovery.stopped(), "invalid seed address");
        }
        seeds[0].m_address.assign("127.0.0.1\0x", 11);
        Check(discovery.start(seeds, "", tls) == NI_INVALID_ARGUMENT, "embedded NUL seed");
        seeds[0].m_address = "127.0.0.1";
        seeds[0].m_port = 0;
        Check(discovery.start(seeds, "", tls) == NI_INVALID_ARGUMENT, "zero port");
        seeds[0].m_port = 65536;
        Check(discovery.start(seeds, "", tls) == NI_INVALID_ARGUMENT, "oversized port");
        seeds.clear();
        Check(discovery.start(seeds, "", tls) == NI_INVALID_ARGUMENT, "empty seeds");
        std::printf("Discovery bounded selection checks=%u PASS\n", checks);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::printf("Discovery checks=%u FAIL %s\n", checks, error.what());
    }
    catch (...)
    {
        std::printf("Discovery checks=%u FAIL escaped exception\n", checks);
    }
    return 1;
}

void NetDiscoveryTableTest::run()
{
    if (check() != 0)
    {
        throw std::runtime_error("NetDiscoveryTableTest failed");
    }
}