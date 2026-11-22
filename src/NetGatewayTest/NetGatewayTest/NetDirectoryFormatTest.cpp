#include "NetDirectoryFormatTest.h"
#include <stdexcept>
#include <algorithm>
#include "NetInterface/NetInterfaceAPI.h"
#include "NetInterface/NetInterfaceAPI.h"
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#ifdef _DEBUG
#include <crtdbg.h>
#endif

/** 隔离启用后测原子解析、等价地址、缓存和故障；DLL路径只使用值/字节/模块内释放的句柄
@return 0通过，1失败
*/
int NetDirectoryFormatTest::check()
{
    uint32_t checks = 0;
    // condition为断言结果，message为静态说明；本测试main捕获失败异常。
    const auto Check = [&checks](bool condition, const char* message)
    {
        ++checks;
        if (!condition)
        {
            std::printf("FAIL %s\n", message);
            throw 1;
        }
    };
    try
    {
#ifdef STATIC_LIB
        const char* validAddresses[] = {"127.0.0.1", "1.2.3.4", "255.255.255.254", "::1", "2001:db8::1",
            "2001:0DB8:0:0:0:0:0:0001", "1:2:3:4:5:6:7:8", "1::", "::ffff:7f00:1"};
        for (size_t index = 0; index < sizeof(validAddresses) / sizeof(validAddresses[0]); ++index)
        {
            Check(NetInterfaceDirectory::address(validAddresses[index]), "valid numeric address rejected");
        }
        const char* invalidAddresses[] = {"", "0.0.0.0", "255.255.255.255", "127.0.0.01", "1.2.3", "1.2.3.4.",
            "1.2.3.256", "::", ":::1", "1:::2", "1::2::3", "1:2:3:4:5:6:7", "1:2:3:4:5:6:7:8::", "::1:",
            "[::1]", "fe80::1%2", "1:2:3:4:5:6:7:gggg", "::ffff:0:0", "::ffff:ffff:ffff", "localhost"};
        for (size_t index = 0; index < sizeof(invalidAddresses) / sizeof(invalidAddresses[0]); ++index)
        {
            Check(!NetInterfaceDirectory::address(invalidAddresses[index]), "invalid numeric address accepted");
        }
        NetInterfaceDirectoryPoint point;
        point.m_name = "node.0123456789abcdef0123456789abcdef";
        point.m_address = "2001:db8::1";
        point.m_port = 12345;
        point.m_capacity = 32;
        point.m_boot = UINT64_C(0xfedcba9876543210);
        std::vector<NetInterfaceDirectoryPoint> points(1, point);
        std::string encoded;
        Check(NetInterfaceDirectory::encode(points, encoded) == NI_OK, "directory encoding");
        points[0].m_name = encoded;
        Check(NetInterfaceDirectory::parse(reinterpret_cast<const uint8_t*>(points[0].m_name.data()),
            static_cast<uint32_t>(points[0].m_name.size()), points) == NI_OK && points.size() == 1 && points[0].m_name == point.m_name,
            "directory destroyed aliased input before parsing");
        Check(NetInterfaceDirectory::encode(points, points[0].m_name) == NI_OK, "encoding destroyed aliased input name");
        Check(NetInterfaceDirectory::parse(reinterpret_cast<const uint8_t*>(points[0].m_name.data()),
            static_cast<uint32_t>(points[0].m_name.size()), points) == NI_OK && points[0].m_boot == point.m_boot,
            "encoding alias did not preserve metadata");
        std::string cache;
        const uint64_t now = UINT64_C(10000000000);
        Check(NetInterfaceDirectory::encodeCache(points, now, cache) == NI_OK, "cache encoding");
        points[0].m_name = cache;
        Check(NetInterfaceDirectory::parseCache(reinterpret_cast<const uint8_t*>(points[0].m_name.data()),
            static_cast<uint32_t>(points[0].m_name.size()), now, points) == NI_OK && points[0].m_name == point.m_name,
            "cache destroyed aliased input before parsing");
        Check(NetInterfaceDirectory::parseCache(reinterpret_cast<const uint8_t*>(cache.data()), static_cast<uint32_t>(cache.size()),
            now + UINT64_C(2592000000), points) == NI_OK, "cache exact thirty-day boundary");
        Check(NetInterfaceDirectory::parseCache(reinterpret_cast<const uint8_t*>(cache.data()), static_cast<uint32_t>(cache.size()),
            now + UINT64_C(2592000001), points) == NI_TIMEOUT && points.empty(), "stale cache retained candidates");
        Check(NetInterfaceDirectory::parseCache(reinterpret_cast<const uint8_t*>(cache.data()), static_cast<uint32_t>(cache.size()),
            now - 300000, points) == NI_OK, "cache exact future allowance");
        Check(NetInterfaceDirectory::parseCache(reinterpret_cast<const uint8_t*>(cache.data()), static_cast<uint32_t>(cache.size()),
            now - 300001, points) == NI_TIMEOUT && points.empty(), "future cache accepted");
        points.assign(2, point);
        points[1].m_name[5] = 'f';
        points[1].m_address = "2001:0DB8:0:0:0:0:0:0001";
        Check(NetInterfaceDirectory::encode(points, encoded) == NI_INVALID_ARGUMENT && encoded.empty(), "equivalent IPv6 endpoints counted twice");
        points[0].m_address = "127.0.0.1";
        points[1].m_address = "::ffff:7f00:1";
        Check(NetInterfaceDirectory::encode(points, encoded) == NI_INVALID_ARGUMENT, "mapped IPv4 endpoint counted twice");
        points[1].m_port++;
        Check(NetInterfaceDirectory::encode(points, encoded) == NI_OK, "different ports incorrectly merged");
        points.assign(64, point);
        const char digits[] = "0123456789abcdef";
        for (uint32_t index = 0; index < 64; ++index)
        {
            points[index].m_name[35] = digits[index / 16];
            points[index].m_name[36] = digits[index % 16];
            points[index].m_port += index;
            points[index].m_boot += index;
        }
        Check(NetInterfaceDirectory::encode(points, encoded) == NI_OK && encoded.size() <= 16384, "full directory encoding");
        std::vector<NetInterfaceDirectoryPoint> parsed;
        Check(NetInterfaceDirectory::parse(reinterpret_cast<const uint8_t*>(encoded.data()), static_cast<uint32_t>(encoded.size()), parsed) == NI_OK &&
            parsed.size() == points.size(), "full directory parse");
        for (uint32_t index = 0; index < 64; ++index)
        {
            Check(parsed[index].m_name == points[index].m_name && parsed[index].m_address == points[index].m_address &&
                parsed[index].m_port == points[index].m_port && parsed[index].m_boot == points[index].m_boot, "full directory identity changed");
        }
        points.push_back(point);
        Check(NetInterfaceDirectory::encode(points, cache) == NI_INVALID_ARGUMENT && cache.empty(), "65th node accepted");
        points.pop_back();
        for (uint32_t size = 0; size < encoded.size(); size += static_cast<uint32_t>(encoded.size() / 64 + 1))
        {
            Check(NetInterfaceDirectory::parse(reinterpret_cast<const uint8_t*>(encoded.data()), size, parsed) < 0 && parsed.empty(),
                "truncated directory retained partial nodes");
        }
        Check(NetInterfaceDirectory::encodeCache(points, now, cache) == NI_OK, "full cache encoding");
        for (size_t offset = 0; offset < cache.size(); offset += cache.size() / 64 + 1)
        {
            cache[offset] ^= 1;
            Check(NetInterfaceDirectory::parseCache(reinterpret_cast<const uint8_t*>(cache.data()), static_cast<uint32_t>(cache.size()), now, parsed) < 0 && parsed.empty(),
                "tampered cache retained nodes");
            cache[offset] ^= 1;
        }
        std::vector<NetGatewayAddress> seeds;
        Check(NetClientDiscovery::parseSeeds("[2001:db8::1]:7,[2001:0DB8:0:0:0:0:0:0001]:7,127.0.0.1:8,[::ffff:7f00:1]:8", seeds) == NI_OK &&
            seeds.size() == 2 && seeds[0].m_address == "2001:db8::1" && seeds[1].m_address == "127.0.0.1", "seed numeric deduplication");
        seeds[0].m_address = "[2001:db8::1]:7,127.0.0.1:8";
        Check(NetClientDiscovery::parseSeeds(seeds[0].m_address, seeds) == NI_OK && seeds.size() == 2,
            "seed parser destroyed aliased text");
        const char* invalidSeeds[] = {",127.0.0.1:1", "127.0.0.1:1,", "127.0.0.1:1,,127.0.0.1:2", "[::1]", "[::1]x:1", "[::1]:",
            "[::1:1", "127.0.0.1:0", "127.0.0.1:65536", "127.0.0.1:123456", "127.0.0.1:-1", "127.0.0.1:1:2", "::1:2", "localhost:7"};
        for (size_t index = 0; index < sizeof(invalidSeeds) / sizeof(invalidSeeds[0]); ++index)
        {
            Check(NetClientDiscovery::parseSeeds(invalidSeeds[index], seeds) == NI_INVALID_ARGUMENT && seeds.empty(), "invalid seed accepted");
        }
        std::string repeated;
        for (uint32_t index = 0; index < 64; ++index)
        {
            if (index != 0) { repeated += ','; }
            repeated += "127.0.0.1:7";
        }
        Check(NetClientDiscovery::parseSeeds(repeated, seeds) == NI_OK && seeds.size() == 1, "64 seed limit or duplicate handling");
        repeated += ",127.0.0.1:7";
        Check(NetClientDiscovery::parseSeeds(repeated, seeds) == NI_OK && seeds.size() == 1, "duplicate seeds beyond 64 use one candidate");
#ifdef _DEBUG
        // 正常编码已预热protobuf默认状态；后续逐个拒绝分配，不将初始化缓存算作泄漏。
        _CrtMemState before;
        _CrtMemState after;
        _CrtMemCheckpoint(&before);
        static uint32_t s_failAt = 0;
        static uint32_t s_seen = 0;
        // 标准CRT钩子参数；仅拒绝第s_failAt次普通分配，其余保持真实分配器。
        const auto Hook = [](int operation, void*, size_t, int block, long, const unsigned char*, int) -> int
        {
            if (operation == _HOOK_ALLOC && block == _NORMAL_BLOCK)
            {
                return ++s_seen == s_failAt ? 0 : 1;
            }
            return 1;
        };
        uint32_t failures = 0;
        for (uint32_t position = 1; position <= 64; ++position)
        {
            std::string output;
            s_seen = 0;
            s_failAt = position;
            _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(Hook);
            const int32_t result = NetInterfaceDirectory::encode(points, output);
            _CrtSetAllocHook(previous);
            if (result != NI_OK)
            {
                ++failures;
                Check(output.empty(), "failed encoding left partial output");
            }
            Check(NetInterfaceDirectory::encode(points, output) == NI_OK, "encoding failed after allocation recovery");
            std::vector<NetInterfaceDirectoryPoint> input;
            s_seen = 0;
            previous = _CrtSetAllocHook(Hook);
            const int32_t parsedResult = NetInterfaceDirectory::parse(reinterpret_cast<const uint8_t*>(output.data()),
                static_cast<uint32_t>(output.size()), input);
            _CrtSetAllocHook(previous);
            Check(parsedResult == NI_OK ? input.size() == 64 : input.empty(), "failed parsing left partial output");
        }
        Check(failures != 0, "allocation failure injection not reached");
        _CrtMemCheckpoint(&after);
        Check(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK],
            "directory failure rollback leaked allocations");
#endif
#else
        NetInterfaceDirectoryNodeData nodes[64] = {};
        const char digits[] = "0123456789abcdef";
        for (uint32_t index = 0; index < 64; ++index)
        {
            nodes[index].m_structSize = sizeof(nodes[index]);
            nodes[index].m_version = 1;
            std::memcpy(nodes[index].m_name, "node.0123456789abcdef0123456789abcdef", 38);
            nodes[index].m_name[35] = digits[index / 16];
            nodes[index].m_name[36] = digits[index % 16];
            std::memcpy(nodes[index].m_address, "2001:db8::1", 12);
            nodes[index].m_port = 20000 + index;
            nodes[index].m_capacity = 32;
            nodes[index].m_boot = UINT64_C(0xfedcba9876543210) + index;
        }
        NetInterfaceDirectoryHandle handle = nullptr;
        Check(NetInterface::directoryCreateNodes(nodes, 64, &handle) == NI_OK && handle != nullptr, "C directory creation");
        std::unique_ptr<NetInterfaceDirectoryOpaque, decltype(&NetInterface::directoryDestroy)> owner(handle, NetInterface::directoryDestroy);
        uint32_t required = 0;
        Check(NetInterface::directoryCache(handle, 1000000, 1, nullptr, 0, &required) == NI_BUFFER_TOO_SMALL && required <= 16432,
            "C cache size query");
        std::vector<uint8_t> bytes(required);
        Check(NetInterface::directoryCache(handle, 1000000, 1, bytes.data(), required, &required) == NI_OK, "C cache encoding");
        NetInterfaceDirectoryHandle parsed = nullptr;
        Check(NetInterface::directoryCreateCache(bytes.data(), required, 1000000, 1, &parsed) == NI_OK && parsed != nullptr,
            "C cache parse");
        std::unique_ptr<NetInterfaceDirectoryOpaque, decltype(&NetInterface::directoryDestroy)> parsedOwner(parsed, NetInterface::directoryDestroy);
        NetInterfaceDirectoryNodeData output[64] = {};
        for (uint32_t index = 0; index < 64; ++index)
        {
            output[index].m_structSize = sizeof(output[index]);
            output[index].m_version = 1;
        }
        uint32_t count = 0;
        Check(NetInterface::directoryCopyNodes(parsed, output, 64, &count) == NI_OK && count == 64, "C directory copy");
        for (uint32_t index = 0; index < 64; ++index)
        {
            Check(std::strcmp(nodes[index].m_name, output[index].m_name) == 0 && nodes[index].m_port == output[index].m_port &&
                nodes[index].m_boot == output[index].m_boot, "C directory values crossed ownership boundary");
        }
        nodes[1].m_port = nodes[0].m_port;
        std::memcpy(nodes[1].m_address, "2001:0DB8:0:0:0:0:0:0001", 24);
        NetInterfaceDirectoryHandle failed = reinterpret_cast<NetInterfaceDirectoryHandle>(1);
        Check(NetInterface::directoryCreateNodes(nodes, 2, &failed) == NI_INVALID_ARGUMENT && failed == nullptr,
            "C equivalent address duplicate accepted");
        for (uint32_t size = 0; size < required; size += required / 32 + 1)
        {
            failed = reinterpret_cast<NetInterfaceDirectoryHandle>(1);
            Check(NetInterface::directoryCreateCache(bytes.data(), size, 1000000, 1, &failed) < 0 && failed == nullptr,
                "C truncated cache produced handle");
        }
        for (uint32_t index = 0; index < required; index += required / 32 + 1)
        {
            bytes[index] ^= 1;
            Check(NetInterface::directoryCreateCache(bytes.data(), required, 1000000, 1, &failed) < 0 && failed == nullptr,
                "C corrupt cache produced handle");
            bytes[index] ^= 1;
        }
#endif

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
        Check(NetInterface::directoryCreateNodes(&node, 1, &handle) == NI_OK, "alias fixture creation");
        std::unique_ptr<NetInterfaceDirectoryOpaque, decltype(&NetInterface::directoryDestroy)> owner(handle, NetInterface::directoryDestroy);
        uint64_t aligned[32];
        std::memset(aligned, 0x5a, sizeof(aligned));
        const uint64_t expected = UINT64_C(0x5a5a5a5a5a5a5a5a);
        uint8_t* bytes = reinterpret_cast<uint8_t*>(aligned);
        Check(NetInterface::directoryCreate(bytes, sizeof(aligned), reinterpret_cast<NetInterfaceDirectoryHandle*>(aligned)) == NI_INVALID_ARGUMENT &&
            aligned[0] == expected, "create handle destroyed input");
        Check(NetInterface::directoryCreateCache(bytes, sizeof(aligned), 1000, 1,
            reinterpret_cast<NetInterfaceDirectoryHandle*>(aligned + 1)) == NI_INVALID_ARGUMENT &&
            aligned[1] == expected, "cache handle destroyed input");
        Check(NetInterface::directoryCache(handle, 1000, 1, bytes, sizeof(aligned), reinterpret_cast<uint32_t*>(aligned + 2)) == NI_INVALID_ARGUMENT &&
            aligned[2] == expected, "cache required overlaps output");
        uint32_t fields[4] = {9, 8, 7, 6};
        char* name = reinterpret_cast<char*>(bytes);
        char* address = reinterpret_cast<char*>(bytes + 64);
        Check(NetInterface::directoryNode(handle, 0, name, 38, name + 10, 40, fields) == NI_INVALID_ARGUMENT &&
            aligned[0] == expected && fields[0] == 9, "node overlapping strings accepted");
        Check(NetInterface::directoryNode(handle, 0, name, 38, address, 40, reinterpret_cast<uint32_t*>(aligned + 1)) == NI_INVALID_ARGUMENT &&
            aligned[1] == expected, "node fields overlap name");
        Check(NetInterface::directoryNode(handle, 0, name, 38, address, 40, reinterpret_cast<uint32_t*>(aligned + 9)) == NI_INVALID_ARGUMENT &&
            aligned[9] == expected, "node fields overlap address");
        Check(NetInterface::directoryNode(handle, 0, name, 38, address, 40, fields) == NI_OK &&
            fields[0] == 12345 && std::strcmp(address, "127.0.0.1") == 0, "alias failure corrupted handle");
    }

        std::printf("NetInterface directory boundary checks=%u PASS\n", checks);
        return 0;
    }
    catch (...)
    {
        std::printf("NetInterface directory boundary checks=%u FAIL\n", checks);
        return 1;
    }
}

void NetDirectoryFormatTest::run()
{
    if (check() != 0)
    {
        throw std::runtime_error("NetDirectoryFormatTest failed");
    }
}