#include "NetInterfaceDirectory.h"
#include "NetInterfaceMacro.h"
#include "CStringManager/CStringManagerAPI.h"
#include "NetInterfaceGatewayTypes.h"
#include "NetInterface.h"
#include "NetInterfaceMessage.h"
#include <cstring>
#include <utility>
#include <array>

/** 校验单个公开候选并保留数值地址键，供整张表检查重复端点
@param [in] point 待检查的候选记录
@param [out] addressKey 成功时写入16字节数值地址
@return 身份、地址、端口、容量及代次全部有效时为true
*/
static bool ValidDirectoryPoint(const NetInterfaceDirectoryPoint& point, uint8_t* addressKey)
{
    return point.m_name.size() == NET_INTERFACE_DIRECTORY_NAME_BYTES && CStringManager::startsWith(point.m_name, "node.") &&
        CStringManager::findFirstNotOf(point.m_name, "0123456789abcdef", 5) == std::string::npos &&
        point.m_port > 0 && point.m_port <= 65535 && point.m_capacity > 0 && point.m_boot != 0 &&
        point.m_address.size() <= 39 && NetInterfaceDirectoryAddress::parse(point.m_address.data(),
        static_cast<uint32_t>(point.m_address.size()), addressKey);
}

/** 验证当前条目与之前已验证条目的唯一性，地址按数值而非拼写比较
@param [in] points 原完整表，至少index+1项
@param [in] index 当前条目号，小于points.size()
@param [in,out] keys 与points等长的16字节去重键，前index项已生成
@return 当前项合法且名称/地址端口未重复
*/
static bool DirectoryUnique(const std::vector<NetInterfaceDirectoryPoint>& points, uint32_t index, std::vector<std::array<uint8_t, 16>>& keys)
{
    const NetInterfaceDirectoryPoint& point = points[index];
    if (!ValidDirectoryPoint(point, keys[index].data()))
    {
        return false;
    }
    for (uint32_t prior = 0; prior < index; ++prior)
    {
        if (point.m_name == points[prior].m_name || (point.m_port == points[prior].m_port &&
            std::memcmp(keys[index].data(), keys[prior].data(), 16) == 0))
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
    uint8_t addressKey[16];
    return ValidDirectoryPoint(point, addressKey);
}

bool NetInterfaceDirectory::number(const ProtoMessage& message, int32_t field, uint64_t& value, int32_t group)
{
    value = 0;
    const Variant* item = message.findValue(field, group);
    if (item == nullptr) { return false; }
    if (item->type() == Variant::UINT64)
    {
        value = NetInterfaceMessage::decodeUint64(*item);
        return true;
    }
    if (item->type() != Variant::INT32) { return false; }
    const int32_t number = NetInterfaceMessage::decodeInt32(*item);
    if (number < 0) { return false; }
    value = static_cast<uint32_t>(number);
    return true;
}

/** 从目录消息复制一个合法的有界文本；分配异常由调用入口按其返回类型处理
@param [in] message 已解析的管理消息
@param [in] field 字段键
@param [out] value 成功时为完整文本，字段无效时清空
@param [in] maximum 允许的最大字节数
@param [in] group 字段所在组
@return 类型、长度及字符均有效时为 true
*/
static bool ReadDirectoryText(const ProtoMessage& message, int32_t field, std::string& value, uint32_t maximum, int32_t group)
{
    const Variant* item = message.findValue(field, group);
    if (item && item->type() == Variant::STRING && item->size() != 0 && item->size() <= maximum)
    {
        value.assign(item->data(), item->size());
        if (CStringManager::findFirstNotOf(value, "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-:") == std::string::npos)
        {
            return true;
        }
    }
    value.clear();
    return false;
}

bool NetInterfaceDirectory::text(const ProtoMessage& message, int32_t field, std::string& value, uint32_t maximum, int32_t group)
{
    try { return ReadDirectoryText(message, field, value, maximum, group); }
    catch (...)
    {
        value.clear();
        return false;
    }
}

int32_t NetInterfaceDirectory::encode(const std::vector<NetInterfaceDirectoryPoint>& points, std::string& output)
{
    if (points.empty())
    {
        output.clear();
        return NI_INVALID_ARGUMENT;
    }
    if (points.size() > NET_INTERFACE_DIRECTORY_BYTES / NET_INTERFACE_DIRECTORY_NAME_BYTES)
    {
        output.clear();
        return NI_LIMIT_EXCEEDED;
    }
    try
    {
        std::vector<std::array<uint8_t, 16>> keys(points.size());
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
        message.setKeyValue(NI_GATEWAY_NODE_COUNT, NetInterfaceMessage::encodeInteger(static_cast<int32_t>(points.size())));
        for (uint32_t index = 0; index < points.size(); ++index)
        {
            const NetInterfaceDirectoryPoint& point = points[index];
            const int32_t group = static_cast<int32_t>(index + 1);
            message.setKeyValue(NI_GATEWAY_NODE_ID, Variant(point.m_name.c_str()), group);
            message.setKeyValue(NI_GATEWAY_ADDRESS, Variant(point.m_address.c_str()), group);
            message.setKeyValue(NI_GATEWAY_PORT, NetInterfaceMessage::encodeInteger(static_cast<int32_t>(point.m_port)), group);
            message.setKeyValue(NI_GATEWAY_CAPACITY, NetInterfaceMessage::encodeInteger(static_cast<uint64_t>(point.m_capacity)), group);
            message.setKeyValue(NI_GATEWAY_BOOT, NetInterfaceMessage::encodeInteger(point.m_boot), group);
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
        if (!message.from(data, size) || message.getCode() != NI_OK ||
            !number(message, NI_GATEWAY_NODE_COUNT, count) || count == 0 || count > size / NET_INTERFACE_DIRECTORY_NAME_BYTES)
        {
            points.clear();
            return NI_PROTOCOL_ERROR;
        }
        std::vector<NetInterfaceDirectoryPoint> parsed;
        parsed.reserve(static_cast<size_t>(count));
        std::vector<std::array<uint8_t, 16>> keys(static_cast<size_t>(count));
        for (int32_t group = 1; group <= static_cast<int32_t>(count); ++group)
        {
            NetInterfaceDirectoryPoint point;
            uint64_t port = 0;
            uint64_t capacity = 0;
            if (!ReadDirectoryText(message, NI_GATEWAY_NODE_ID, point.m_name, NET_INTERFACE_DIRECTORY_NAME_BYTES, group) ||
                !ReadDirectoryText(message, NI_GATEWAY_ADDRESS, point.m_address, 39, group) ||
                !number(message, NI_GATEWAY_PORT, port, group) || port > 65535 ||
                !number(message, NI_GATEWAY_CAPACITY, capacity, group) || capacity > UINT32_MAX ||
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