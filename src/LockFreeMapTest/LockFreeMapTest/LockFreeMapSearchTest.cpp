#include "LockFreeMapSearchTest.h"
#include "LockFreeMap.h"
#include <algorithm>
#include <functional>
#include <limits>
#include <map>
#include <vector>

bool LockFreeMapSearchGroupLess::operator()(const int64_t& left, const int64_t& right) const
{
    return left / 7 < right / 7;
}

/** 正反交替查询，覆盖命中、未命中、边界、借用及前次位置提示
@param [in] map 被测容器
@param [in] expected 独立标准容器
@param [in] queries 包含极值及相邻键的固定查询集合
@return 全部结果、边界键和值及回调次数一致时true
*/
template <typename Key, typename Compare>
static bool CheckSearchResults(const LockFreeMap<Key, int, Compare>& map,
    const std::map<Key, int, Compare>& expected, const std::vector<Key>& queries)
{
    if (map.size() != expected.size())
    {
        return false;
    }
    for (size_t index = 0; index < queries.size() * 2; ++index)
    {
        const size_t position = index < queries.size() ? index : queries.size() * 2 - index - 1;
        const Key key = queries[position];
        typename std::map<Key, int, Compare>::const_iterator item = expected.find(key);
        const bool exists = item != expected.end();
        int value = -1;
        if (map.get(key, value) != exists || (exists && value != item->second) ||
            map.contains(key) != exists || map.count(key) != (exists ? 1 : 0))
        {
            return false;
        }
        unsigned calls = 0;
        int borrowed = -1;
        const LockFreeMapStatus status = map.lockFreeMapVisit(key, [&](const int& current)
        {
            ++calls;
            borrowed = current;
        });
        if (status != (exists ? LOCK_FREE_MAP_OK : LOCK_FREE_MAP_NOT_FOUND) ||
            calls != (exists ? 1U : 0U) || (exists && borrowed != item->second))
        {
            return false;
        }
        Key found = Key();
        item = expected.lower_bound(key);
        const bool lower = item != expected.end();
        if (map.lowerBound(key, found, value) != lower ||
            (lower && (found != item->first || value != item->second)))
        {
            return false;
        }
        item = expected.upper_bound(key);
        const bool upper = item != expected.end();
        if (map.upperBound(key, found, value) != upper ||
            (upper && (found != item->first || value != item->second)))
        {
            return false;
        }
    }
    return true;
}

/** 在空表、奇偶规模、删除视图和重插视图上对照完整查询集合
@return 每一步状态与标准容器一致时true；分配异常由外层入口转换为失败
*/
template <typename Key, typename Compare>
static bool CheckSearchType()
{
    const Key minimum = (std::numeric_limits<Key>::min)();
    const Key maximum = (std::numeric_limits<Key>::max)();
    std::vector<Key> keys;
    keys.push_back(minimum);
    keys.push_back(maximum);
    keys.push_back(static_cast<Key>(minimum + 1));
    keys.push_back(static_cast<Key>(maximum - 1));
    for (int index = 0; index < 65; ++index)
    {
        keys.push_back(static_cast<Key>(index * 2 - 65));
    }
    std::vector<Key> queries;
    for (size_t index = 0; index < keys.size(); ++index)
    {
        const Key key = keys[index];
        queries.push_back(key);
        if (key != minimum)
        {
            queries.push_back(static_cast<Key>(key - 1));
        }
        if (key != maximum)
        {
            queries.push_back(static_cast<Key>(key + 1));
        }
    }
    const size_t sizes[] = { 0, 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65, 69 };
    for (size_t sizeIndex = 0; sizeIndex < sizeof(sizes) / sizeof(sizes[0]); ++sizeIndex)
    {
        LockFreeMap<Key, int, Compare> map;
        std::map<Key, int, Compare> expected;
        const size_t size = sizes[sizeIndex];
        for (size_t index = 0; index < size; ++index)
        {
            const int value = static_cast<int>(index) + 17;
            if (!map.set(keys[index], value))
            {
                return false;
            }
            expected[keys[index]] = value;
        }
        if (!CheckSearchResults(map, expected, queries))
        {
            return false;
        }
        for (size_t index = 0; index < size; index += 3)
        {
            const bool removed = expected.erase(keys[index]) != 0;
            if (map.erase(keys[index]) != removed)
            {
                return false;
            }
        }
        if (!CheckSearchResults(map, expected, queries))
        {
            return false;
        }
        for (size_t index = 0; index < size; index += 3)
        {
            const int value = static_cast<int>(index) + 900;
            if (!map.set(keys[index], value))
            {
                return false;
            }
            expected[keys[index]] = value;
        }
        if (!CheckSearchResults(map, expected, queries))
        {
            return false;
        }
        map.clear();
        expected.clear();
        if (!CheckSearchResults(map, expected, queries))
        {
            return false;
        }
    }
    return true;
}

/** 覆盖多级分段边界、末键删除后的提示及变化后的逻辑视图
@return 大表及反复删除重插均与标准容器一致时true
*/
static bool CheckSearchLargeRange()
{
    const int sizes[] = { 127, 128, 129, 255, 256, 257, 1023, 1024, 1025, 4095, 4096, 4097 };
    for (size_t sizeIndex = 0; sizeIndex < sizeof(sizes) / sizeof(sizes[0]); ++sizeIndex)
    {
        const int count = sizes[sizeIndex];
        LockFreeMap<int, int> map;
        std::map<int, int> expected;
        std::vector<int> queries;
        for (int key = -1; key <= count * 2; ++key)
        {
            queries.push_back(key);
        }
        for (int index = 0; index < count; ++index)
        {
            if (!map.set(index * 2, index))
            {
                return false;
            }
            expected[index * 2] = index;
        }
        if (!CheckSearchResults(map, expected, queries))
        {
            return false;
        }
        // 用公开重入读取登记第二条操作记录，随后覆盖写会走与并发调用相同的搜索分支。
        bool nested = false;
        if (map.lockFreeMapVisit(0, [&](const int& value)
        {
            int next = -1;
            nested = value == 0 && map.get(2, next) && next == 1;
        }) != LOCK_FREE_MAP_OK || !nested)
        {
            return false;
        }
        const int steps[] = { -17, -16, -8, -4, -3, -2, 0, 1, 2, 3, 4, 8, 16, 17 };
        int position = 0;
        for (size_t stepIndex = 0; stepIndex < sizeof(steps) / sizeof(steps[0]); ++stepIndex)
        {
            for (int round = 0; round < count + 17; ++round)
            {
                position = (position + count + steps[stepIndex]) % count;
                const int value = round + static_cast<int>(stepIndex) * 10000;
                int observed = -1;
                if (!map.set(position * 2, value) || !map.get(position * 2, observed) || observed != value)
                {
                    return false;
                }
                expected[position * 2] = value;
            }
        }
        if (!CheckSearchResults(map, expected, queries))
        {
            return false;
        }
        const int removed[] = { 0, count / 4, count / 2, count - 1 };
        for (size_t position = 0; position < sizeof(removed) / sizeof(removed[0]); ++position)
        {
            const int key = removed[position] * 2;
            if (!map.erase(key) || expected.erase(key) != 1 ||
                !CheckSearchResults(map, expected, queries) || !map.set(key, key))
            {
                return false;
            }
            expected[key] = key;
        }
        // 单独连续操作末键，使位置提示在旧视图末项与新视图末尾之间转换。
        const int last = (count - 1) * 2;
        for (int round = 0; round < 32; ++round)
        {
            int value = -1, found = -1;
            if (!map.get(last, value) || !map.erase(last) || map.contains(last) ||
                map.lowerBound(last, found, value) || !map.set(last, round) ||
                !map.get(last, value) || value != round)
            {
                return false;
            }
            expected[last] = round;
        }
        if (!CheckSearchResults(map, expected, queries))
        {
            return false;
        }
    }
    return true;
}

/** 对照整数极值附近的等距键、同规模替换及稀疏化后的查询
@return 数值位置提示始终经过实际键验证时true
*/
template <typename Key>
static bool CheckSearchNumericBounds()
{
    const Key minimum = (std::numeric_limits<Key>::min)();
    const Key maximum = (std::numeric_limits<Key>::max)();
    const Key origins[] = { minimum, static_cast<Key>(maximum - 512), static_cast<Key>(19) };
    LockFreeMap<Key, int> map;
    std::map<Key, int> expected;
    std::vector<Key> queries;
    for (size_t originIndex = 0; originIndex < sizeof(origins) / sizeof(origins[0]); ++originIndex)
    {
        map.clear();
        expected.clear();
        queries.clear();
        for (int index = 0; index < 128; ++index)
        {
            const Key key = static_cast<Key>(origins[originIndex] + static_cast<Key>(index * 3));
            if (!map.set(key, index))
            {
                return false;
            }
            expected[key] = index;
            queries.push_back(key);
        }
        // 固定置换打乱间隙查询，避免全部由相邻访问提示完成；两侧边界由std::map独立判定。
        for (size_t index = 0; index < 128; ++index)
        {
            const size_t position = index * 73 % 128;
            queries.push_back(static_cast<Key>(origins[originIndex] + static_cast<Key>(position * 3 + 1)));
            queries.push_back(static_cast<Key>(origins[originIndex] + static_cast<Key>(position * 3 + 2)));
        }
        // 先验证命中及间隙，再插入间隙键并删除首项，保持总数不变。
        if (!CheckSearchResults(map, expected, queries))
        {
            return false;
        }
        const Key replaced = static_cast<Key>(origins[originIndex] + 1);
        if (!map.erase(origins[originIndex]) || !map.set(replaced, 731))
        {
            return false;
        }
        expected.erase(origins[originIndex]);
        expected[replaced] = 731;
        queries.push_back(replaced);
        queries.push_back(minimum);
        queries.push_back(maximum);
        if (!CheckSearchResults(map, expected, queries))
        {
            return false;
        }
        // 静止期释放后复用同一个实例，旧数值坐标不能代替键验证。
        map.clearAndFreeMemory();
        expected.clear();
        if (!CheckSearchResults(map, expected, queries))
        {
            return false;
        }
    }
    return true;
}

/** 用不等距键和乱序查询验证精确搜索，覆盖小表边界及删除后同规模复用
@return 所有公开查询与独立标准容器一致时true
*/
static bool CheckSearchSparse()
{
    const uint32_t sizes[] = { 1, 2, 3, 4, 5, 7, 15, 31, 63, 127, 128, 129, 255, 256, 257, 4096 };
    for (size_t sizeIndex = 0; sizeIndex < sizeof(sizes) / sizeof(sizes[0]); ++sizeIndex)
    {
        const uint32_t count = sizes[sizeIndex];
        LockFreeMap<uint32_t, int> map;
        std::map<uint32_t, int> expected;
        std::vector<uint32_t> queries;
        for (uint32_t index = 0; index < count; ++index)
        {
            // 奇数乘法在无符号32位域中不重复，标准容器独立确定排序和等价关系。
            const uint32_t key = index * 2654435761U;
            if (!map.set(key, static_cast<int>(index)))
            {
                return false;
            }
            expected[key] = static_cast<int>(index);
            queries.push_back(key);
            queries.push_back(key + 1U);
        }
        queries.push_back((std::numeric_limits<uint32_t>::max)());
        uint32_t random = 2463534242U;
        for (size_t index = queries.size() - 1; index > 0; --index)
        {
            random ^= random << 13;
            random ^= random >> 17;
            random ^= random << 5;
            std::swap(queries[index], queries[random % (index + 1)]);
        }
        if (!CheckSearchResults(map, expected, queries))
        {
            return false;
        }
        const uint32_t removed = expected.rbegin()->first;
        if (!map.erase(removed) || expected.erase(removed) != 1 ||
            !CheckSearchResults(map, expected, queries))
        {
            return false;
        }
        if (!map.set(removed, 731))
        {
            return false;
        }
        expected[removed] = 731;
        if (!CheckSearchResults(map, expected, queries))
        {
            return false;
        }
    }
    return true;
}

bool LockFreeMapSearchTest()
{
    try
    {
        return CheckSearchType<int32_t, std::less<int32_t>>() &&
            CheckSearchType<int64_t, std::less<int64_t>>() &&
            CheckSearchType<uint64_t, std::less<uint64_t>>() &&
            CheckSearchType<int64_t, std::greater<int64_t>>() &&
            CheckSearchType<int64_t, LockFreeMapSearchGroupLess>() && CheckSearchLargeRange() &&
            CheckSearchNumericBounds<int32_t>() && CheckSearchNumericBounds<int64_t>() &&
            CheckSearchNumericBounds<uint64_t>() && CheckSearchSparse();
    }
    catch (...)
    {
        return false;
    }
}