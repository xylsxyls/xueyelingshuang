#include "NetInterfaceMacro.h"
#include "NetClientDiscovery.h"
#include "NetInterfaceDirectory.h"
#include <utility>
#include <cstring>

NetClientDiscovery::~NetClientDiscovery()
{

}

bool NetClientDiscovery::validPoint(const NetClientDiscoveryPoint& endpoint)
{
    uint8_t bytes[16];
    return addressKey(endpoint, bytes);
}

bool NetClientDiscovery::addressKey(const NetClientDiscoveryPoint& endpoint, uint8_t* key)
{
    if (key == nullptr || endpoint.m_structSize != sizeof(endpoint) || endpoint.m_version != 1 || endpoint.m_port == 0 || endpoint.m_port > 65535)
    {
        return false;
    }
    const char* end = static_cast<const char*>(std::memchr(endpoint.m_address, 0, sizeof(endpoint.m_address)));
    uint8_t bytes[16];
    if (end == nullptr || !NetInterfaceDirectoryAddress::parse(endpoint.m_address,
        static_cast<uint32_t>(end - endpoint.m_address), bytes))
    {
        return false;
    }
    std::memcpy(key, bytes, sizeof(bytes));
    return true;
}

/** 写输出前检查别名，使用地址差避免末端地址相加溢出
@param [in] first 第一段地址
@param [in] firstSize 第一段长度
@param [in] second 第二段地址
@param [in] secondSize 第二段长度
@return 两个非空范围相交
*/
static bool DiscoveryRangesOverlap(const void* first, size_t firstSize, const void* second, size_t secondSize)
{
    if (first == nullptr || second == nullptr || firstSize == 0 || secondSize == 0)
    {
        return false;
    }
    const uintptr_t left = reinterpret_cast<uintptr_t>(first);
    const uintptr_t right = reinterpret_cast<uintptr_t>(second);
    return left <= right ? right - left < firstSize : left - right < secondSize;
}

int32_t NetClientDiscovery::parseSeeds(const std::string& text, NetClientDiscoveryPoint* points,
    uint32_t capacity, uint32_t& count)
{
    const size_t arraySize = capacity <= 64 ? capacity * sizeof(NetClientDiscoveryPoint) : 0;
    if (DiscoveryRangesOverlap(&count, sizeof(count), points, arraySize) ||
        DiscoveryRangesOverlap(&count, sizeof(count), &text, sizeof(text)) ||
        DiscoveryRangesOverlap(&count, sizeof(count), text.data(), text.size() + 1) ||
        DiscoveryRangesOverlap(points, arraySize, &text, sizeof(text)) ||
        DiscoveryRangesOverlap(points, arraySize, text.data(), text.size() + 1))
    {
        return NI_INVALID_ARGUMENT;
    }
    count = 0;
    if (capacity > 64 || (points == nullptr && capacity != 0))
    {
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        std::vector<NetGatewayAddress> parsed;
        const int32_t result = parseSeeds(text, parsed);
        if (result != NI_OK)
        {
            return result;
        }
        if (capacity < parsed.size())
        {
            count = static_cast<uint32_t>(parsed.size());
            return NI_BUFFER_TOO_SMALL;
        }
        for (uint32_t index = 0; index < parsed.size(); ++index)
        {
            if (points[index].m_structSize != sizeof(NetClientDiscoveryPoint) || points[index].m_version != 1)
            {
                return NI_INVALID_ARGUMENT;
            }
        }
        for (uint32_t index = 0; index < parsed.size(); ++index)
        {
            NetClientDiscoveryPoint point;
            std::memset(&point, 0, sizeof(point));
            point.m_structSize = sizeof(point);
            point.m_version = 1;
            std::memcpy(point.m_address, parsed[index].m_address.c_str(), parsed[index].m_address.size() + 1);
            point.m_port = parsed[index].m_port;
            points[index] = point;
        }
        count = static_cast<uint32_t>(parsed.size());
        return NI_OK;
    }
    catch (...)
    {
        return NI_NO_MEMORY;
    }
}

int32_t NetClientDiscovery::parseSeeds(const std::string& text, std::vector<NetGatewayAddress>& points)
{
    if (text.empty() || text.size() > 4096 || text.back() == ',')
    {
        points.clear();
        return NI_INVALID_ARGUMENT;
    }
    try
    {
        std::vector<NetGatewayAddress> parsed;
        uint8_t keys[64][16];
        uint32_t count = 0;
        size_t offset = 0;
        while (offset < text.size())
        {
            const size_t comma = text.find(',', offset);
            const size_t end = comma == std::string::npos ? text.size() : comma;
            size_t begin = offset;
            size_t addressEnd = text.find(text[begin] == '[' ? ']' : ':', begin);
            size_t portBegin = addressEnd + 1;
            if (text[begin] == '[')
            {
                ++begin;
                ++portBegin;
            }
            if (++count > 64 || offset == end || addressEnd == std::string::npos || addressEnd <= begin ||
                addressEnd >= end || portBegin >= end || text[portBegin - 1] != ':' || end - portBegin > 5)
            {
                points.clear();
                return NI_INVALID_ARGUMENT;
            }
            NetGatewayAddress point;
            point.m_address.assign(text.data() + begin, addressEnd - begin);
            uint8_t key[16];
            if (!NetInterfaceDirectoryAddress::parse(point.m_address.data(), static_cast<uint32_t>(point.m_address.size()), key))
            {
                points.clear();
                return NI_INVALID_ARGUMENT;
            }
            point.m_port = 0;
            for (size_t index = portBegin; index < end; ++index)
            {
                if (text[index] < '0' || text[index] > '9')
                {
                    points.clear();
                    return NI_INVALID_ARGUMENT;
                }
                point.m_port = point.m_port * 10 + text[index] - '0';
            }
            if (point.m_port == 0 || point.m_port > 65535)
            {
                points.clear();
                return NI_INVALID_ARGUMENT;
            }
            bool duplicate = false;
            for (size_t index = 0; index < parsed.size(); ++index)
            {
                duplicate = duplicate || (point.m_port == parsed[index].m_port && std::memcmp(key, keys[index], 16) == 0);
            }
            if (!duplicate)
            {
                std::memcpy(keys[parsed.size()], key, 16);
                parsed.push_back(std::move(point));
            }
            offset = end + 1;
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