#include "NetInterfaceDirectory.h"
#include "NetInterfaceMacro.h"
#include "NetInterfaceGatewayTypes.h"
#include "NetInterface.h"
#include "NetInterfaceCredit.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include <cstring>
#include <utility>

/** 验证当前条目与之前已验证条目的唯一性，地址按数值而非拼写比较
@param [in] points 原完整表，至少index+1项
@param [in] index 当前条目号，小于64
@param [in,out] keys 64个16字节去重键，前index项已生成
@return 当前项合法且名称/地址端口未重复
*/
static bool DirectoryUnique(const std::vector<NetInterfaceDirectoryPoint>& points, uint32_t index, uint8_t keys[64][16])
{
    const NetInterfaceDirectoryPoint& point = points[index];
    if (!NetInterfaceDirectory::valid(point) || !NetInterfaceDirectoryAddress::parse(point.m_address.data(),
        static_cast<uint32_t>(point.m_address.size()), keys[index]))
    {
        return false;
    }
    for (uint32_t prior = 0; prior < index; ++prior)
    {
        if (point.m_name == points[prior].m_name || (point.m_port == points[prior].m_port &&
            std::memcmp(keys[index], keys[prior], 16) == 0))
        {
            return false;
        }
    }
    return true;
}

bool NetInterfaceDirectoryAddress::parse(const char* value, uint32_t size, uint8_t* bytes)
{
    if (value == nullptr || bytes == nullptr || size == 0 || size > 39)
    {
        return false;
    }
    std::memset(bytes, 0, 16);
    if (std::memchr(value, ':', size) == nullptr)
    {
        uint32_t offset = 0;
        uint32_t all = 0;
        for (uint32_t group = 0; group < 4; ++group)
        {
            const uint32_t begin = offset;
            uint32_t number = 0;
            while (offset < size && value[offset] != '.')
            {
                const char digit = value[offset++];
                if (digit < '0' || digit > '9' || offset - begin > 3)
                {
                    return false;
                }
                number = number * 10 + digit - '0';
            }
            if (offset == begin || number > 255 || (offset - begin > 1 && value[begin] == '0') ||
                (group == 3 ? offset != size : offset == size))
            {
                return false;
            }
            bytes[12 + group] = static_cast<uint8_t>(number);
            all = (all << 8) | number;
            ++offset;
        }
        bytes[10] = bytes[11] = 0xff;
        return all != 0 && all != UINT32_MAX;
    }
    uint16_t groups[8] = {};
    uint32_t count = 0;
    uint32_t compressed = 9;
    uint32_t offset = 0;
    if (value[0] == ':')
    {
        if (size < 2 || value[1] != ':')
        {
            return false;
        }
        compressed = 0;
        offset = 2;
    }
    while (offset < size)
    {
        const uint32_t begin = offset;
        uint32_t word = 0;
        while (offset < size && value[offset] != ':')
        {
            const char digit = value[offset++];
            uint32_t number = 0;
            if (digit >= '0' && digit <= '9')
            {
                number = digit - '0';
            }
            else if (digit >= 'a' && digit <= 'f')
            {
                number = digit - 'a' + 10;
            }
            else if (digit >= 'A' && digit <= 'F')
            {
                number = digit - 'A' + 10;
            }
            else
            {
                return false;
            }
            if (offset - begin > 4)
            {
                return false;
            }
            word = (word << 4) | number;
        }
        if (offset == begin || count == 8)
        {
            return false;
        }
        groups[count++] = static_cast<uint16_t>(word);
        if (offset == size)
        {
            break;
        }
        ++offset;
        if (offset < size && value[offset] == ':')
        {
            if (compressed != 9)
            {
                return false;
            }
            compressed = count;
            ++offset;
        }
        else if (offset == size)
        {
            return false;
        }
    }
    if (compressed == 9 ? count != 8 : count >= 8)
    {
        return false;
    }
    bool nonzero = false;
    for (uint32_t index = 0; index < count; ++index)
    {
        const uint32_t target = compressed != 9 && index >= compressed ? index + 8 - count : index;
        bytes[target * 2] = static_cast<uint8_t>(groups[index] >> 8);
        bytes[target * 2 + 1] = static_cast<uint8_t>(groups[index]);
        nonzero = nonzero || groups[index] != 0;
    }
    bool mapped = bytes[10] == 0xff && bytes[11] == 0xff;
    for (uint32_t index = 0; index < 10; ++index)
    {
        mapped = mapped && bytes[index] == 0;
    }
    if (mapped)
    {
        const uint32_t last = (static_cast<uint32_t>(bytes[12]) << 24) | (static_cast<uint32_t>(bytes[13]) << 16) |
            (static_cast<uint32_t>(bytes[14]) << 8) | bytes[15];
        return last != 0 && last != UINT32_MAX;
    }
    return nonzero;
}

NetInterfaceDirectoryPoint::NetInterfaceDirectoryPoint() :
m_port(0),
m_capacity(0),
m_boot(0)
{

}

bool NetInterfaceDirectory::address(const std::string& value)
{
    uint8_t bytes[16];
    return value.size() <= 39 && NetInterfaceDirectoryAddress::parse(value.data(), static_cast<uint32_t>(value.size()), bytes);
}

bool NetInterfaceDirectory::valid(const NetInterfaceDirectoryPoint& point)
{
    return point.m_name.size() == 37 && point.m_name.compare(0, 5, "node.") == 0 &&
        point.m_name.find_first_not_of("0123456789abcdef", 5) == std::string::npos && address(point.m_address) &&
        point.m_port > 0 && point.m_port <= 65535 && point.m_capacity > 0 && point.m_capacity <= 256 && point.m_boot != 0;
}

bool NetInterfaceDirectory::number(const ProtoMessage& message, int32_t field, uint64_t& value, int32_t group)
{
    value = 0;
    try
    {
        Variant item;
        if (!message.getKeyValue(field, item, group))
        {
            return false;
        }
        if (item.type() == Variant::UINT64)
        {
            value = static_cast<uint64_t>(item);
            return true;
        }
        if (item.type() != Variant::INT32 || static_cast<int32_t>(item) < 0)
        {
            return false;
        }
        value = static_cast<uint32_t>(static_cast<int32_t>(item));
        return true;
    }
    catch (...)
    {
        return false;
    }
}

bool NetInterfaceDirectory::text(const ProtoMessage& message, int32_t field, std::string& value, uint32_t maximum, int32_t group)
{
    value.clear();
    try
    {
        Variant item;
        if (!message.getKeyValue(field, item, group) || item.type() != Variant::STRING)
        {
            return false;
        }
        std::string parsed = static_cast<std::string>(item);
        if (parsed.empty() || parsed.size() > maximum || parsed.find('\0') != std::string::npos ||
            parsed.find_first_not_of("0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-:") != std::string::npos)
        {
            return false;
        }
        value.swap(parsed);
        return true;
    }
    catch (...)
    {
        return false;
    }
}

int32_t NetInterfaceDirectory::encode(const std::vector<NetInterfaceDirectoryPoint>& points, std::string& output)
{
    if (points.empty() || points.size() > NET_INTERFACE_DIRECTORY_NODES)
    {
        output.clear();
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        uint8_t keys[64][16];
        for (uint32_t index = 0; index < points.size(); ++index)
        {
            if (!DirectoryUnique(points, index, keys))
            {
                output.clear();
                return NI_INVALID_ARGUMENT;
            }
        }
        ProtoMessage message;
        message.setCode(NI_OK);
        message.setKeyValue(NI_GATEWAY_NODE_COUNT, Variant(static_cast<int32_t>(points.size())));
        for (uint32_t index = 0; index < points.size(); ++index)
        {
            const NetInterfaceDirectoryPoint& point = points[index];
            const int32_t group = static_cast<int32_t>(index + 1);
            message.setKeyValue(NI_GATEWAY_NODE_ID, Variant(point.m_name.c_str()), group);
            message.setKeyValue(NI_GATEWAY_ADDRESS, Variant(point.m_address.c_str()), group);
            message.setKeyValue(NI_GATEWAY_PORT, Variant(static_cast<int32_t>(point.m_port)), group);
            message.setKeyValue(NI_GATEWAY_CAPACITY, Variant(static_cast<int32_t>(point.m_capacity)), group);
            message.setKeyValue(NI_GATEWAY_BOOT, Variant(point.m_boot), group);
        }
        std::string encoded;
        if (!message.toString(encoded))
        {
            output.clear();
            return NI_PROTOCOL_ERROR;
        }
        if (encoded.size() > NET_INTERFACE_DIRECTORY_BYTES)
        {
            output.clear();
            return NI_LIMIT_EXCEEDED;
        }
        output.swap(encoded);
        return NI_OK;
    }
    catch (...)
    {
        output.clear();
        return NI_NO_MEMORY;
    }
}

int32_t NetInterfaceDirectory::parse(const uint8_t* data, uint32_t size, std::vector<NetInterfaceDirectoryPoint>& points)
{
    if (data == nullptr || size == 0 || size > NET_INTERFACE_DIRECTORY_BYTES)
    {
        points.clear();
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        // 输入可能借用points内部字符串；完成读取前不能清空原拥有者。
        ProtoMessage message;
        uint64_t count = 0;
        if (!message.from(std::string(reinterpret_cast<const char*>(data), size)) || message.getCode() != NI_OK ||
            !number(message, NI_GATEWAY_NODE_COUNT, count) || count == 0 || count > NET_INTERFACE_DIRECTORY_NODES)
        {
            points.clear();
            return NI_PROTOCOL_ERROR;
        }
        std::vector<NetInterfaceDirectoryPoint> parsed;
        parsed.reserve(static_cast<size_t>(count));
        uint8_t keys[64][16];
        for (int32_t group = 1; group <= static_cast<int32_t>(count); ++group)
        {
            NetInterfaceDirectoryPoint point;
            uint64_t port = 0;
            uint64_t capacity = 0;
            if (!text(message, NI_GATEWAY_NODE_ID, point.m_name, 37, group) ||
                !text(message, NI_GATEWAY_ADDRESS, point.m_address, 39, group) ||
                !number(message, NI_GATEWAY_PORT, port, group) || port > 65535 ||
                !number(message, NI_GATEWAY_CAPACITY, capacity, group) || capacity > 256 ||
                !number(message, NI_GATEWAY_BOOT, point.m_boot, group))
            {
                points.clear();
                return NI_PROTOCOL_ERROR;
            }
            point.m_port = static_cast<uint32_t>(port);
            point.m_capacity = static_cast<uint32_t>(capacity);
            parsed.push_back(std::move(point));
            if (!DirectoryUnique(parsed, static_cast<uint32_t>(group - 1), keys))
            {
                points.clear();
                return NI_PROTOCOL_ERROR;
            }
        }
        points.swap(parsed);
        return NI_OK;
    }
    catch (...)
    {
        points.clear();
        return NI_NO_MEMORY;
    }
}

int32_t NetInterfaceDirectory::encodeCache(const std::vector<NetInterfaceDirectoryPoint>& points, uint64_t utcMs, std::string& output)
{
    if (utcMs == 0)
    {
        output.clear();
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        std::string bytes;
        const int32_t result = encode(points, bytes);
        if (result != NI_OK)
        {
            output.clear();
            return result;
        }
        std::string cache(16, '\0');
        cache.reserve(bytes.size() + 48);
        std::memcpy(&cache[0], "NGC3", 4);
        for (uint32_t index = 0; index < 8; ++index)
        {
            cache[4 + index] = static_cast<char>(utcMs >> (56 - index * 8));
        }
        for (uint32_t index = 0; index < 4; ++index)
        {
            cache[12 + index] = static_cast<char>(bytes.size() >> (24 - index * 8));
        }
        cache += bytes;
        uint8_t hash[32];
        const int32_t hashed = NetInterface::sha256(reinterpret_cast<const uint8_t*>(cache.data()), static_cast<uint32_t>(cache.size()), hash, 32);
        if (hashed != NI_OK)
        {
            output.clear();
            return hashed;
        }
        cache.append(reinterpret_cast<const char*>(hash), sizeof(hash));
        output.swap(cache);
        return NI_OK;
    }
    catch (...)
    {
        output.clear();
        return NI_NO_MEMORY;
    }
}

int32_t NetInterfaceDirectory::parseCache(const uint8_t* data, uint32_t size, uint64_t utcMs, std::vector<NetInterfaceDirectoryPoint>& points)
{
    int32_t error = NI_OK;
    if (data == nullptr || size <= 48 || size > NET_INTERFACE_DIRECTORY_BYTES + 48 || utcMs == 0)
    {
        error = NI_INVALID_ARGUMENT;
    }
    else if (std::memcmp(data, "NGC3", 4) != 0)
    {
        error = NI_PROTOCOL_ERROR;
    }
    else
    {
        uint64_t saved = 0;
        uint32_t length = 0;
        for (uint32_t index = 0; index < 8; ++index)
        {
            saved = (saved << 8) | data[4 + index];
        }
        for (uint32_t index = 0; index < 4; ++index)
        {
            length = (length << 8) | data[12 + index];
        }
        uint8_t hash[32];
        if (length != size - 48 || NetInterface::sha256(data, size - 32, hash, 32) != NI_OK || std::memcmp(hash, data + size - 32, 32) != 0)
        {
            error = NI_PROTOCOL_ERROR;
        }
        else if (saved == 0 || (saved > utcMs ? saved - utcMs > 300000 : utcMs - saved > UINT64_C(2592000000)))
        {
            error = NI_TIMEOUT;
        }
        else
        {
            return parse(data + 16, length, points);
        }
    }
    points.clear();
    return error;
}

#if 0 // DIRECTORY_VALUES_UNIT_TEST_BEGIN
#include "NetInterface.h"
#include <cstring>
#include <cstdio>
#include <memory>
#include <type_traits>
#include <cstddef>

/** 值记录输入复制、版本/终止符/重复项校验及同模块句柄释放。
@return 0通过，1失败
*/
int main()
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
    VALUE_CHECK(NetInterface::directoryCreateNodes(&node, 65, &handle) == NI_INVALID_ARGUMENT && handle == nullptr);
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
        case 6: invalid.m_capacity = 257; break;
        case 7: invalid.m_boot = 0; break;
        default: invalid.m_address[0] = 0; break;
        }
        handle = reinterpret_cast<NetInterfaceDirectoryHandle>(1);
        VALUE_CHECK(NetInterface::directoryCreateNodes(&invalid, 1, &handle) == NI_INVALID_ARGUMENT && handle == nullptr);
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
    VALUE_CHECK(NetInterface::directoryCopyNodes(handle, &copied, 65, &count) == NI_INVALID_ARGUMENT && count == 0);
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

#endif // DIRECTORY_VALUES_UNIT_TEST_END

// NET_INTERFACE_DIRECTORY_UNIT_BEGIN
//#include "NetInterface.h"
//#include "NetInterfaceDirectory.h"
//#include "NetClientDiscovery.h"
//#include <cstdio>
//#include <cstring>
//#include <memory>
//#include <string>
//#include <vector>
//#ifdef _DEBUG
//#include <crtdbg.h>
//#endif
//
///** 隔离启用后测原子解析、等价地址、缓存和故障；DLL路径只使用值/字节/模块内释放的句柄
//@return 0通过，1失败
//*/
//int main()
//{
//    uint32_t checks = 0;
//    // condition为断言结果，message为静态说明；本测试main捕获失败异常。
//    const auto Check = [&checks](bool condition, const char* message)
//    {
//        ++checks;
//        if (!condition)
//        {
//            std::printf("FAIL %s\n", message);
//            throw 1;
//        }
//    };
//    try
//    {
//#ifdef STATIC_LIB
//        const char* validAddresses[] = {"127.0.0.1", "1.2.3.4", "255.255.255.254", "::1", "2001:db8::1",
//            "2001:0DB8:0:0:0:0:0:0001", "1:2:3:4:5:6:7:8", "1::", "::ffff:7f00:1"};
//        for (size_t index = 0; index < sizeof(validAddresses) / sizeof(validAddresses[0]); ++index)
//        {
//            Check(NetInterfaceDirectory::address(validAddresses[index]), "valid numeric address rejected");
//        }
//        const char* invalidAddresses[] = {"", "0.0.0.0", "255.255.255.255", "127.0.0.01", "1.2.3", "1.2.3.4.",
//            "1.2.3.256", "::", ":::1", "1:::2", "1::2::3", "1:2:3:4:5:6:7", "1:2:3:4:5:6:7:8::", "::1:",
//            "[::1]", "fe80::1%2", "1:2:3:4:5:6:7:gggg", "::ffff:0:0", "::ffff:ffff:ffff", "localhost"};
//        for (size_t index = 0; index < sizeof(invalidAddresses) / sizeof(invalidAddresses[0]); ++index)
//        {
//            Check(!NetInterfaceDirectory::address(invalidAddresses[index]), "invalid numeric address accepted");
//        }
//        NetInterfaceDirectoryPoint point;
//        point.m_name = "node.0123456789abcdef0123456789abcdef";
//        point.m_address = "2001:db8::1";
//        point.m_port = 12345;
//        point.m_capacity = 32;
//        point.m_boot = UINT64_C(0xfedcba9876543210);
//        std::vector<NetInterfaceDirectoryPoint> points(1, point);
//        std::string encoded;
//        Check(NetInterfaceDirectory::encode(points, encoded) == NI_OK, "directory encoding");
//        points[0].m_name = encoded;
//        Check(NetInterfaceDirectory::parse(reinterpret_cast<const uint8_t*>(points[0].m_name.data()),
//            static_cast<uint32_t>(points[0].m_name.size()), points) == NI_OK && points.size() == 1 && points[0].m_name == point.m_name,
//            "directory destroyed aliased input before parsing");
//        Check(NetInterfaceDirectory::encode(points, points[0].m_name) == NI_OK, "encoding destroyed aliased input name");
//        Check(NetInterfaceDirectory::parse(reinterpret_cast<const uint8_t*>(points[0].m_name.data()),
//            static_cast<uint32_t>(points[0].m_name.size()), points) == NI_OK && points[0].m_boot == point.m_boot,
//            "encoding alias did not preserve metadata");
//        std::string cache;
//        const uint64_t now = UINT64_C(10000000000);
//        Check(NetInterfaceDirectory::encodeCache(points, now, cache) == NI_OK, "cache encoding");
//        points[0].m_name = cache;
//        Check(NetInterfaceDirectory::parseCache(reinterpret_cast<const uint8_t*>(points[0].m_name.data()),
//            static_cast<uint32_t>(points[0].m_name.size()), now, points) == NI_OK && points[0].m_name == point.m_name,
//            "cache destroyed aliased input before parsing");
//        Check(NetInterfaceDirectory::parseCache(reinterpret_cast<const uint8_t*>(cache.data()), static_cast<uint32_t>(cache.size()),
//            now + UINT64_C(2592000000), points) == NI_OK, "cache exact thirty-day boundary");
//        Check(NetInterfaceDirectory::parseCache(reinterpret_cast<const uint8_t*>(cache.data()), static_cast<uint32_t>(cache.size()),
//            now + UINT64_C(2592000001), points) == NI_TIMEOUT && points.empty(), "stale cache retained candidates");
//        Check(NetInterfaceDirectory::parseCache(reinterpret_cast<const uint8_t*>(cache.data()), static_cast<uint32_t>(cache.size()),
//            now - 300000, points) == NI_OK, "cache exact future allowance");
//        Check(NetInterfaceDirectory::parseCache(reinterpret_cast<const uint8_t*>(cache.data()), static_cast<uint32_t>(cache.size()),
//            now - 300001, points) == NI_TIMEOUT && points.empty(), "future cache accepted");
//        points.assign(2, point);
//        points[1].m_name[5] = 'f';
//        points[1].m_address = "2001:0DB8:0:0:0:0:0:0001";
//        Check(NetInterfaceDirectory::encode(points, encoded) == NI_INVALID_ARGUMENT && encoded.empty(), "equivalent IPv6 endpoints counted twice");
//        points[0].m_address = "127.0.0.1";
//        points[1].m_address = "::ffff:7f00:1";
//        Check(NetInterfaceDirectory::encode(points, encoded) == NI_INVALID_ARGUMENT, "mapped IPv4 endpoint counted twice");
//        points[1].m_port++;
//        Check(NetInterfaceDirectory::encode(points, encoded) == NI_OK, "different ports incorrectly merged");
//        points.assign(64, point);
//        const char digits[] = "0123456789abcdef";
//        for (uint32_t index = 0; index < 64; ++index)
//        {
//            points[index].m_name[35] = digits[index / 16];
//            points[index].m_name[36] = digits[index % 16];
//            points[index].m_port += index;
//            points[index].m_boot += index;
//        }
//        Check(NetInterfaceDirectory::encode(points, encoded) == NI_OK && encoded.size() <= 16384, "full directory encoding");
//        std::vector<NetInterfaceDirectoryPoint> parsed;
//        Check(NetInterfaceDirectory::parse(reinterpret_cast<const uint8_t*>(encoded.data()), static_cast<uint32_t>(encoded.size()), parsed) == NI_OK &&
//            parsed.size() == points.size(), "full directory parse");
//        for (uint32_t index = 0; index < 64; ++index)
//        {
//            Check(parsed[index].m_name == points[index].m_name && parsed[index].m_address == points[index].m_address &&
//                parsed[index].m_port == points[index].m_port && parsed[index].m_boot == points[index].m_boot, "full directory identity changed");
//        }
//        points.push_back(point);
//        Check(NetInterfaceDirectory::encode(points, cache) == NI_INVALID_ARGUMENT && cache.empty(), "65th node accepted");
//        points.pop_back();
//        for (uint32_t size = 0; size < encoded.size(); size += static_cast<uint32_t>(encoded.size() / 64 + 1))
//        {
//            Check(NetInterfaceDirectory::parse(reinterpret_cast<const uint8_t*>(encoded.data()), size, parsed) < 0 && parsed.empty(),
//                "truncated directory retained partial nodes");
//        }
//        Check(NetInterfaceDirectory::encodeCache(points, now, cache) == NI_OK, "full cache encoding");
//        for (size_t offset = 0; offset < cache.size(); offset += cache.size() / 64 + 1)
//        {
//            cache[offset] ^= 1;
//            Check(NetInterfaceDirectory::parseCache(reinterpret_cast<const uint8_t*>(cache.data()), static_cast<uint32_t>(cache.size()), now, parsed) < 0 && parsed.empty(),
//                "tampered cache retained nodes");
//            cache[offset] ^= 1;
//        }
//        std::vector<NetGatewayAddress> seeds;
//        Check(NetClientDiscovery::parseSeeds("[2001:db8::1]:7,[2001:0DB8:0:0:0:0:0:0001]:7,127.0.0.1:8,[::ffff:7f00:1]:8", seeds) == NI_OK &&
//            seeds.size() == 2 && seeds[0].m_address == "2001:db8::1" && seeds[1].m_address == "127.0.0.1", "seed numeric deduplication");
//        seeds[0].m_address = "[2001:db8::1]:7,127.0.0.1:8";
//        Check(NetClientDiscovery::parseSeeds(seeds[0].m_address, seeds) == NI_OK && seeds.size() == 2,
//            "seed parser destroyed aliased text");
//        const char* invalidSeeds[] = {",127.0.0.1:1", "127.0.0.1:1,", "127.0.0.1:1,,127.0.0.1:2", "[::1]", "[::1]x:1", "[::1]:",
//            "[::1:1", "127.0.0.1:0", "127.0.0.1:65536", "127.0.0.1:123456", "127.0.0.1:-1", "127.0.0.1:1:2", "::1:2", "localhost:7"};
//        for (size_t index = 0; index < sizeof(invalidSeeds) / sizeof(invalidSeeds[0]); ++index)
//        {
//            Check(NetClientDiscovery::parseSeeds(invalidSeeds[index], seeds) == NI_INVALID_ARGUMENT && seeds.empty(), "invalid seed accepted");
//        }
//        std::string repeated;
//        for (uint32_t index = 0; index < 64; ++index)
//        {
//            if (index != 0) { repeated += ','; }
//            repeated += "127.0.0.1:7";
//        }
//        Check(NetClientDiscovery::parseSeeds(repeated, seeds) == NI_OK && seeds.size() == 1, "64 seed limit or duplicate handling");
//        repeated += ",127.0.0.1:7";
//        Check(NetClientDiscovery::parseSeeds(repeated, seeds) == NI_INVALID_ARGUMENT && seeds.empty(), "65 seed inputs accepted after deduplication");
//#ifdef _DEBUG
//        // 正常编码已预热protobuf默认状态；后续逐个拒绝分配，不将初始化缓存算作泄漏。
//        _CrtMemState before;
//        _CrtMemState after;
//        _CrtMemCheckpoint(&before);
//        static uint32_t s_failAt = 0;
//        static uint32_t s_seen = 0;
//        // 标准CRT钩子参数；仅拒绝第s_failAt次普通分配，其余保持真实分配器。
//        const auto Hook = [](int operation, void*, size_t, int block, long, const unsigned char*, int) -> int
//        {
//            if (operation == _HOOK_ALLOC && block == _NORMAL_BLOCK)
//            {
//                return ++s_seen == s_failAt ? 0 : 1;
//            }
//            return 1;
//        };
//        uint32_t failures = 0;
//        for (uint32_t position = 1; position <= 64; ++position)
//        {
//            std::string output;
//            s_seen = 0;
//            s_failAt = position;
//            _CRT_ALLOC_HOOK previous = _CrtSetAllocHook(Hook);
//            const int32_t result = NetInterfaceDirectory::encode(points, output);
//            _CrtSetAllocHook(previous);
//            if (result != NI_OK)
//            {
//                ++failures;
//                Check(output.empty(), "failed encoding left partial output");
//            }
//            Check(NetInterfaceDirectory::encode(points, output) == NI_OK, "encoding failed after allocation recovery");
//            std::vector<NetInterfaceDirectoryPoint> input;
//            s_seen = 0;
//            previous = _CrtSetAllocHook(Hook);
//            const int32_t parsedResult = NetInterfaceDirectory::parse(reinterpret_cast<const uint8_t*>(output.data()),
//                static_cast<uint32_t>(output.size()), input);
//            _CrtSetAllocHook(previous);
//            Check(parsedResult == NI_OK ? input.size() == 64 : input.empty(), "failed parsing left partial output");
//        }
//        Check(failures != 0, "allocation failure injection not reached");
//        _CrtMemCheckpoint(&after);
//        Check(before.lCounts[_NORMAL_BLOCK] == after.lCounts[_NORMAL_BLOCK] && before.lSizes[_NORMAL_BLOCK] == after.lSizes[_NORMAL_BLOCK],
//            "directory failure rollback leaked allocations");
//#endif
//#else
//        NetInterfaceDirectoryNodeData nodes[64] = {};
//        const char digits[] = "0123456789abcdef";
//        for (uint32_t index = 0; index < 64; ++index)
//        {
//            nodes[index].m_structSize = sizeof(nodes[index]);
//            nodes[index].m_version = 1;
//            std::memcpy(nodes[index].m_name, "node.0123456789abcdef0123456789abcdef", 38);
//            nodes[index].m_name[35] = digits[index / 16];
//            nodes[index].m_name[36] = digits[index % 16];
//            std::memcpy(nodes[index].m_address, "2001:db8::1", 12);
//            nodes[index].m_port = 20000 + index;
//            nodes[index].m_capacity = 32;
//            nodes[index].m_boot = UINT64_C(0xfedcba9876543210) + index;
//        }
//        NetInterfaceDirectoryHandle handle = nullptr;
//        Check(NetInterface::directoryCreateNodes(nodes, 64, &handle) == NI_OK && handle != nullptr, "C directory creation");
//        std::unique_ptr<NetInterfaceDirectoryOpaque, decltype(&NetInterface::directoryDestroy)> owner(handle, NetInterface::directoryDestroy);
//        uint32_t required = 0;
//        Check(NetInterface::directoryCache(handle, 1000000, 1, nullptr, 0, &required) == NI_BUFFER_TOO_SMALL && required <= 16432,
//            "C cache size query");
//        std::vector<uint8_t> bytes(required);
//        Check(NetInterface::directoryCache(handle, 1000000, 1, bytes.data(), required, &required) == NI_OK, "C cache encoding");
//        NetInterfaceDirectoryHandle parsed = nullptr;
//        Check(NetInterface::directoryCreateCache(bytes.data(), required, 1000000, 1, &parsed) == NI_OK && parsed != nullptr,
//            "C cache parse");
//        std::unique_ptr<NetInterfaceDirectoryOpaque, decltype(&NetInterface::directoryDestroy)> parsedOwner(parsed, NetInterface::directoryDestroy);
//        NetInterfaceDirectoryNodeData output[64] = {};
//        for (uint32_t index = 0; index < 64; ++index)
//        {
//            output[index].m_structSize = sizeof(output[index]);
//            output[index].m_version = 1;
//        }
//        uint32_t count = 0;
//        Check(NetInterface::directoryCopyNodes(parsed, output, 64, &count) == NI_OK && count == 64, "C directory copy");
//        for (uint32_t index = 0; index < 64; ++index)
//        {
//            Check(std::strcmp(nodes[index].m_name, output[index].m_name) == 0 && nodes[index].m_port == output[index].m_port &&
//                nodes[index].m_boot == output[index].m_boot, "C directory values crossed ownership boundary");
//        }
//        nodes[1].m_port = nodes[0].m_port;
//        std::memcpy(nodes[1].m_address, "2001:0DB8:0:0:0:0:0:0001", 24);
//        NetInterfaceDirectoryHandle failed = reinterpret_cast<NetInterfaceDirectoryHandle>(1);
//        Check(NetInterface::directoryCreateNodes(nodes, 2, &failed) == NI_INVALID_ARGUMENT && failed == nullptr,
//            "C equivalent address duplicate accepted");
//        for (uint32_t size = 0; size < required; size += required / 32 + 1)
//        {
//            failed = reinterpret_cast<NetInterfaceDirectoryHandle>(1);
//            Check(NetInterface::directoryCreateCache(bytes.data(), size, 1000000, 1, &failed) < 0 && failed == nullptr,
//                "C truncated cache produced handle");
//        }
//        for (uint32_t index = 0; index < required; index += required / 32 + 1)
//        {
//            bytes[index] ^= 1;
//            Check(NetInterface::directoryCreateCache(bytes.data(), required, 1000000, 1, &failed) < 0 && failed == nullptr,
//                "C corrupt cache produced handle");
//            bytes[index] ^= 1;
//        }
//#endif
//
//    {
//        NetInterfaceDirectoryNodeData node = {};
//        node.m_structSize = sizeof(node);
//        node.m_version = 1;
//        std::memcpy(node.m_name, "node.0123456789abcdef0123456789abcdef", 38);
//        std::memcpy(node.m_address, "127.0.0.1", 10);
//        node.m_port = 12345;
//        node.m_capacity = 32;
//        node.m_boot = 1;
//        NetInterfaceDirectoryHandle handle = nullptr;
//        Check(NetInterface::directoryCreateNodes(&node, 1, &handle) == NI_OK, "alias fixture creation");
//        std::unique_ptr<NetInterfaceDirectoryOpaque, decltype(&NetInterface::directoryDestroy)> owner(handle, NetInterface::directoryDestroy);
//        uint64_t aligned[32];
//        std::memset(aligned, 0x5a, sizeof(aligned));
//        const uint64_t expected = UINT64_C(0x5a5a5a5a5a5a5a5a);
//        uint8_t* bytes = reinterpret_cast<uint8_t*>(aligned);
//        Check(NetInterface::directoryCreate(bytes, sizeof(aligned), reinterpret_cast<NetInterfaceDirectoryHandle*>(aligned)) == NI_INVALID_ARGUMENT &&
//            aligned[0] == expected, "create handle destroyed input");
//        Check(NetInterface::directoryCreateCache(bytes, sizeof(aligned), 1000, 1,
//            reinterpret_cast<NetInterfaceDirectoryHandle*>(aligned + 1)) == NI_INVALID_ARGUMENT &&
//            aligned[1] == expected, "cache handle destroyed input");
//        Check(NetInterface::directoryCache(handle, 1000, 1, bytes, sizeof(aligned), reinterpret_cast<uint32_t*>(aligned + 2)) == NI_INVALID_ARGUMENT &&
//            aligned[2] == expected, "cache required overlaps output");
//        uint32_t fields[4] = {9, 8, 7, 6};
//        char* name = reinterpret_cast<char*>(bytes);
//        char* address = reinterpret_cast<char*>(bytes + 64);
//        Check(NetInterface::directoryNode(handle, 0, name, 38, name + 10, 40, fields) == NI_INVALID_ARGUMENT &&
//            aligned[0] == expected && fields[0] == 9, "node overlapping strings accepted");
//        Check(NetInterface::directoryNode(handle, 0, name, 38, address, 40, reinterpret_cast<uint32_t*>(aligned + 1)) == NI_INVALID_ARGUMENT &&
//            aligned[1] == expected, "node fields overlap name");
//        Check(NetInterface::directoryNode(handle, 0, name, 38, address, 40, reinterpret_cast<uint32_t*>(aligned + 9)) == NI_INVALID_ARGUMENT &&
//            aligned[9] == expected, "node fields overlap address");
//        Check(NetInterface::directoryNode(handle, 0, name, 38, address, 40, fields) == NI_OK &&
//            fields[0] == 12345 && std::strcmp(address, "127.0.0.1") == 0, "alias failure corrupted handle");
//    }
//
//        std::printf("NetInterface directory boundary checks=%u PASS\n", checks);
//        return 0;
//    }
//    catch (...)
//    {
//        std::printf("NetInterface directory boundary checks=%u FAIL\n", checks);
//        return 1;
//    }
//}
// NET_INTERFACE_DIRECTORY_UNIT_END