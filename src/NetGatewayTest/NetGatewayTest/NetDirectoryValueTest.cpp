#include "NetDirectoryValueTest.h"
#include "NetGatewaySuiteCases.h"
#include <stdexcept>
#include "NetInterface/NetInterfaceAPI.h"
#include <cstring>
#include <cstdio>
#include <memory>
#include <type_traits>
#include <cstddef>

/** 值记录输入复制、版本/终止符/重复项校验及同模块句柄释放。
@return 0通过，1失败
*/
int NetDirectoryValueTest::check()
{
    uint32_t assertions = 0;
#define VALUE_CHECK(condition) { ++assertions; if (!(condition)) { std::printf("FAIL line %u: %s\n", static_cast<uint32_t>(__LINE__), #condition); return 1; } }
    static_assert(std::is_standard_layout<NetInterfaceDirectoryNodeData>::value, "directory ABI is a value record");
    static_assert(std::is_trivially_copyable<NetInterfaceDirectoryNodeData>::value, "directory ABI has no allocation");
    NetInterfaceDirectoryNodeData node = {};
    node.m_structSize = sizeof(node);
    node.m_version = 1;
    std::memcpy(node.m_name, "node.0123456789abcdef0123456789abcdef", 38);
    std::memcpy(node.m_address, "2001:db8::1234", 14);
    node.m_port = 23456;
    node.m_capacity = 32;
    node.m_boot = UINT64_C(0xfedcba9876543210);
    VALUE_CHECK(NetInterface::directoryCreateNodes(&node, 1, reinterpret_cast<NetInterfaceDirectoryHandle*>(&node.m_boot)) == NI_INVALID_ARGUMENT &&
        node.m_boot == UINT64_C(0xfedcba9876543210));
    NetInterfaceDirectoryHandle handle = nullptr;
    VALUE_CHECK(NetInterface::directoryCreateNodes(nullptr, 1, &handle) == NI_INVALID_ARGUMENT && handle == nullptr);
    VALUE_CHECK(NetInterface::directoryCreateNodes(&node, 0, &handle) == NI_INVALID_ARGUMENT && handle == nullptr);
    VALUE_CHECK(NetInterface::directoryCreateNodes(&node, UINT32_MAX, &handle) == NI_INVALID_ARGUMENT && handle == nullptr);
    VALUE_CHECK(NetInterface::directoryCreateNodes(&node, 1, nullptr) == NI_INVALID_ARGUMENT);
    for (uint32_t test = 0; test < 9; ++test)
    {
        NetInterfaceDirectoryNodeData invalid = node;
        switch (test)
        {
        case 0: invalid.m_structSize = 0; break;
        case 1: invalid.m_version = 2; break;
        case 2: std::memset(invalid.m_name, 'a', sizeof(invalid.m_name)); break;
        case 3: std::memset(invalid.m_address, '1', sizeof(invalid.m_address)); break;
        case 4: invalid.m_port = 0; break;
        case 5: invalid.m_port = 65536; break;
        case 6: invalid.m_capacity = 0; break;
        case 7: invalid.m_boot = 0; break;
        default: invalid.m_address[0] = 0; break;
        }
        handle = reinterpret_cast<NetInterfaceDirectoryHandle>(1);
        VALUE_CHECK(NetInterface::directoryCreateNodes(&invalid, 1, &handle) == NI_INVALID_ARGUMENT && handle == nullptr);
    }
    {
        std::vector<NetInterfaceDirectoryNodeData> many(80, node);
        for (uint32_t index = 0; index < many.size(); ++index)
        {
            std::sprintf(many[index].m_name, "node.%032x", index);
            many[index].m_port = 10000 + index;
            many[index].m_capacity = UINT32_MAX;
        }
        NetInterfaceDirectoryHandle large = nullptr;
        VALUE_CHECK(NetInterface::directoryCreateNodes(many.data(), static_cast<uint32_t>(many.size()), &large) == NI_OK);
        std::unique_ptr<NetInterfaceDirectoryOpaque, decltype(&NetInterface::directoryDestroy)> largeOwner(large, NetInterface::directoryDestroy);
        uint32_t count = 0;
        VALUE_CHECK(NetInterface::directoryCount(large, &count) == NI_OK && count == 80);
        VALUE_CHECK(NetInterface::directoryCopyNodes(large, many.data(), 80, &count) == NI_OK && many[79].m_capacity == UINT32_MAX);
    }
    NetInterfaceDirectoryNodeData duplicate[2] = {node, node};
    VALUE_CHECK(NetInterface::directoryCreateNodes(duplicate, 2, &handle) == NI_INVALID_ARGUMENT && handle == nullptr);
    VALUE_CHECK(NetInterface::directoryCreateNodes(&node, 1, &handle) == NI_OK && handle != nullptr);
    std::unique_ptr<NetInterfaceDirectoryOpaque, decltype(&NetInterface::directoryDestroy)> owner(handle, NetInterface::directoryDestroy);
    std::memset(&node, 0, sizeof(node));
    uint32_t count = 0;
    VALUE_CHECK(NetInterface::directoryCount(handle, &count) == NI_OK && count == 1);
    VALUE_CHECK(NetInterface::directoryCopyNodes(handle, nullptr, 0, &count) == NI_BUFFER_TOO_SMALL && count == 1);
    NetInterfaceDirectoryNodeData copied = {};
    std::memset(copied.m_name, 0x5a, sizeof(copied.m_name));
    VALUE_CHECK(NetInterface::directoryCopyNodes(handle, &copied, 0, &count) == NI_BUFFER_TOO_SMALL && count == 1 && copied.m_name[0] == 0x5a);
    VALUE_CHECK(NetInterface::directoryCopyNodes(handle, &copied, 1, &count) == NI_INVALID_ARGUMENT && count == 0 && copied.m_name[0] == 0x5a);
    copied.m_structSize = sizeof(copied);
    copied.m_version = 1;
    VALUE_CHECK(NetInterface::directoryCopyNodes(handle, &copied, 1, &count) == NI_OK && count == 1 && copied.m_boot == UINT64_C(0xfedcba9876543210));
    VALUE_CHECK(NetInterface::directoryCopyNodes(handle, &copied, 1, &copied.m_capacity) == NI_INVALID_ARGUMENT && copied.m_capacity == 32);
    for (size_t index = offsetof(NetInterfaceDirectoryNodeData, m_address) + sizeof(copied.m_address);
        index < offsetof(NetInterfaceDirectoryNodeData, m_port); ++index)
    {
        VALUE_CHECK(reinterpret_cast<const uint8_t*>(&copied)[index] == 0);
    }
    VALUE_CHECK(NetInterface::directoryCopyNodes(handle, nullptr, 1, &count) == NI_INVALID_ARGUMENT && count == 0);
    VALUE_CHECK(NetInterface::directoryCopyNodes(handle, &copied, UINT32_MAX, &count) == NI_INVALID_ARGUMENT && count == 0);
    char name[38] = {};
    char address[40] = {};
    uint32_t fields[4] = {};
    VALUE_CHECK(NetInterface::directoryNode(handle, 0, name, sizeof(name), address, sizeof(address), fields) == NI_OK);
    VALUE_CHECK(std::strcmp(name, "node.0123456789abcdef0123456789abcdef") == 0 && std::strcmp(address, "2001:db8::1234") == 0);
    VALUE_CHECK(fields[0] == 23456 && fields[1] == 32 && fields[2] == 0x76543210U && fields[3] == 0xfedcba98U);
    const uint64_t utc = UINT64_C(1800000000000);
    uint8_t cache[16432] = {};
    uint32_t required = 99;
    std::memset(cache, 0x5a, sizeof(cache));
    VALUE_CHECK(NetInterface::directoryCache(handle, static_cast<uint32_t>(utc & UINT64_C(0xffffffff)), static_cast<uint32_t>(utc >> 32),
        cache, 1, &required) == NI_BUFFER_TOO_SMALL && required > 48 && cache[0] == 0x5a && cache[1] == 0x5a);
    VALUE_CHECK(NetInterface::directoryCache(handle, static_cast<uint32_t>(utc & UINT64_C(0xffffffff)), static_cast<uint32_t>(utc >> 32),
        cache, sizeof(cache), &required) == NI_OK);
    NetInterfaceDirectoryHandle loaded = nullptr;
    VALUE_CHECK(NetInterface::directoryCreateCache(cache, required, static_cast<uint32_t>(utc & UINT64_C(0xffffffff)), static_cast<uint32_t>(utc >> 32), &loaded) == NI_OK);
    std::unique_ptr<NetInterfaceDirectoryOpaque, decltype(&NetInterface::directoryDestroy)> loadedOwner(loaded, NetInterface::directoryDestroy);
    VALUE_CHECK(NetInterface::directoryNode(loaded, 0, name, sizeof(name), address, sizeof(address), fields) == NI_OK && fields[3] == 0xfedcba98U);
    cache[20] ^= 1;
    loaded = reinterpret_cast<NetInterfaceDirectoryHandle>(1);
    VALUE_CHECK(NetInterface::directoryCreateCache(cache, required, static_cast<uint32_t>(utc & UINT64_C(0xffffffff)), static_cast<uint32_t>(utc >> 32), &loaded) == NI_PROTOCOL_ERROR && loaded == nullptr);
    VALUE_CHECK(NetInterface::directoryCount(nullptr, &count) == NI_INVALID_ARGUMENT && count == 0);
    NetInterface::directoryDestroy(nullptr);
    std::printf("Directory value ABI assertions=%u PASS\n", assertions);
    return 0;
#undef VALUE_CHECK
}


void NetDirectoryValueTest::run()
{
    netTestCheckpoint();
    if (check() != 0)
    {
        throw std::runtime_error("Directory value ABI, validation and ownership failed");
    }
}