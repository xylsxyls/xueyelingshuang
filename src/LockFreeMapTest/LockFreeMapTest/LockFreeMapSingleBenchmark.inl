#pragma once
#include <algorithm>
#include <iostream>
#ifdef _MSC_VER
#include <intrin.h>
#endif

inline void LockFreeMapSingleBenchmark::barrier(const void* container)
{
#ifdef _MSC_VER
    (void)container;
    _ReadWriteBarrier();
#else
    __asm__ __volatile__("" : : "g"(container) : "memory");
#endif
}

template <size_t Operation>
bool LockFreeMapSingleBenchmark::exercise(LockFreeMapSingleContainer& map,
    const std::vector<uint32_t>& queries, uint64_t iterations, uint64_t& checksum)
{
    const size_t keys = queries.size();
    checksum = 0;
    for (uint64_t index = 0; index < iterations; ++index)
    {
        barrier(&map);
        const uint32_t key = Operation == 0 ? static_cast<uint32_t>(index & (keys - 1)) * 2 :
            queries[static_cast<size_t>(index) & (keys - 1)];
        uint64_t value = 0;
        uint32_t foundKey = 0;
        bool success = true;
#ifdef LOCK_FREE_MAP_SINGLE_STD
        if (Operation <= 2)
        {
            map[key] = index + 1;
            value = index + 1;
        }
        else if (Operation == 3)
        {
            success = !map.insert(std::make_pair(key, uint64_t(17))).second;
        }
        else if (Operation == 4 || Operation == 5 || Operation == 6 || Operation == 13)
        {
            const uint32_t lookup = key + (Operation == 5 ? 1 : 0);
            const LockFreeMapSingleContainer::const_iterator item = map.find(lookup);
            success = (item != map.end()) == (Operation != 5);
            value = item == map.end() ? 0 : item->second;
        }
        else if (Operation == 7)
        {
            value = map.find(key) != map.end() ? 1 : 0;
        }
        else if (Operation == 8)
        {
            value = map.count(key);
        }
        else if (Operation == 9 || Operation == 10)
        {
            LockFreeMapSingleContainer::const_iterator item = Operation == 9 ? map.begin() : --map.end();
            foundKey = item->first;
            value = item->second;
        }
        else if (Operation == 11 || Operation == 12)
        {
            const LockFreeMapSingleContainer::const_iterator item = Operation == 11 ?
                map.lower_bound(key + 1) : map.upper_bound(key);
            success = (item != map.end()) == (key != (keys - 1) * 2);
            if (item != map.end())
            {
                foundKey = item->first;
                value = item->second;
            }
        }
        else if (Operation == 14)
        {
            success = map.erase(key + 1) == 0;
        }
        else if (Operation == 15 || Operation == 16)
        {
            if (Operation == 15)
            {
                success = map.erase(key) == 1;
            }
            else
            {
                const LockFreeMapSingleContainer::iterator item = map.find(key);
                success = item != map.end();
                if (success)
                {
                    value = item->second;
                    map.erase(item);
                }
            }
            success = insert(map, key) && success;
        }
        else if (Operation == 17)
        {
            for (LockFreeMapSingleContainer::const_iterator item = map.begin(); item != map.end(); ++item)
            {
                value += item->first + item->second;
            }
        }
#else
        if (Operation <= 2)
        {
            if (Operation == 2)
            {
                map[key] = index + 1;
            }
            else
            {
                success = map.set(key, index + 1);
            }
            value = index + 1;
        }
        else if (Operation == 3)
        {
            success = !map.insert(key, uint64_t(17));
        }
        else if (Operation == 4 || Operation == 5)
        {
            success = map.get(key + (Operation == 5 ? 1 : 0), value) == (Operation != 5);
        }
        else if (Operation == 6)
        {
            value = map.value(key, 0);
        }
        else if (Operation == 7)
        {
            value = map.contains(key) ? 1 : 0;
        }
        else if (Operation == 8)
        {
            value = map.count(key);
        }
        else if (Operation == 9 || Operation == 10)
        {
            success = Operation == 9 ? map.first(foundKey, value) : map.last(foundKey, value);
        }
        else if (Operation == 11 || Operation == 12)
        {
            success = (Operation == 11 ? map.lowerBound(key + 1, foundKey, value) :
                map.upperBound(key, foundKey, value)) == (key != (keys - 1) * 2);
        }
        else if (Operation == 13)
        {
            // 回调只累加借用值，不让引用逃出当前调用
            success = map.lockFreeMapVisit(key, [&value](const uint64_t& borrowed) -> void
            {
                value = borrowed;
            }) == LOCK_FREE_MAP_OK;
        }
        else if (Operation == 14)
        {
            success = !map.erase(key + 1);
        }
        else if (Operation == 15 || Operation == 16)
        {
            success = Operation == 15 ? map.erase(key) :
                map.lockFreeMapExtract(key, value) == LOCK_FREE_MAP_COMMITTED;
            success = insert(map, key) && success;
        }
        else if (Operation == 17)
        {
            // 累加整张表中的每个键值；遍历结果参与最终校验
            success = map.forEach([&value](const uint32_t& itemKey, const uint64_t& itemValue) -> void
            {
                value += itemKey + itemValue;
            }) == keys;
        }
#endif
        if (Operation == 18)
        {
#ifdef LOCK_FREE_MAP_SINGLE_STD
            std::vector<uint32_t> items;
            items.reserve(map.size());
            for (LockFreeMapSingleContainer::const_iterator item = map.begin(); item != map.end(); ++item)
            {
                items.push_back(item->first);
            }
#else
            const std::vector<uint32_t> items = map.keys();
#endif
            barrier(items.data());
            success = items.size() == keys;
            value = success ? items.front() + items[keys / 2] + items.back() : 0;
        }
        else if (Operation == 19)
        {
#ifdef LOCK_FREE_MAP_SINGLE_STD
            std::vector<uint64_t> items;
            items.reserve(map.size());
            for (LockFreeMapSingleContainer::const_iterator item = map.begin(); item != map.end(); ++item)
            {
                items.push_back(item->second);
            }
#else
            const std::vector<uint64_t> items = map.values();
#endif
            barrier(items.data());
            success = items.size() == keys;
            value = success ? items.front() + items[keys / 2] + items.back() : 0;
        }
        else if (Operation == 20)
        {
#ifdef LOCK_FREE_MAP_SINGLE_STD
            std::vector<std::pair<uint32_t, uint64_t>> items;
            items.reserve(map.size());
            for (LockFreeMapSingleContainer::const_iterator item = map.begin(); item != map.end(); ++item)
            {
                items.emplace_back(item->first, item->second);
            }
#else
            const std::vector<std::pair<uint32_t, uint64_t>> items = map.toVector();
#endif
            barrier(items.data());
            success = items.size() == keys;
            value = success ? items.front().first + items.front().second + items[keys / 2].first +
                items[keys / 2].second + items.back().first + items.back().second : 0;
        }
        else if (Operation == 21 || Operation == 22)
        {
            // 每轮包含从空表插入全部键及清空，后续回收工作也进入稳定态计时
            for (size_t item = 0; item < keys; ++item)
            {
                success = insert(map, static_cast<uint32_t>(item * 2)) && success;
            }
            value = map.size();
            success = value == keys && success;
#ifdef LOCK_FREE_MAP_SINGLE_STD
            map.clear();
#else
            if (Operation == 21)
            {
                map.clear();
            }
            else
            {
                map.clearAndFreeMemory();
            }
#endif
            success = map.empty() && success;
        }
        if (!success)
        {
            return false;
        }
        checksum += value + foundKey;
    }
    return true;
}

template <size_t Operation>
bool LockFreeMapSingleBenchmark::sample(const char* name, uint32_t keys, uint64_t iterations)
{
    LockFreeMapSingleContainer map;
    std::vector<uint32_t> queries(keys);
    std::vector<uint64_t> expected(Operation >= 21 ? 0 : keys);
    for (uint32_t index = 0; index < keys; ++index)
    {
        queries[index] = index * 2;
        if (Operation < 21)
        {
            expected[index] = index * 2 + 17;
            if (!insert(map, index * 2))
            {
                return false;
            }
        }
    }
    // 固定种子的 Fisher-Yates 排列；生成和求模均在计时区外
    uint32_t random = 2463534242U;
    for (size_t index = keys - 1; index > 0; --index)
    {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        std::swap(queries[index], queries[random % (index + 1)]);
    }
    uint64_t count = 1;
    uint64_t checksum = 0;
    uint64_t elapsed = 0;
    // 校准结果也必须可观察，否则编译器能删除只读预热，仅保留计时和空循环
    static volatile uint64_t s_warmupChecksum = 0;
    do
    {
        const uint64_t start = LockFreeMapBenchmarkClock::nanoseconds();
        if (!exercise<Operation>(map, queries, count, checksum))
        {
            return false;
        }
        const uint64_t end = LockFreeMapBenchmarkClock::nanoseconds();
        s_warmupChecksum = checksum;
        if (start == 0 || end < start)
        {
            return false;
        }
        elapsed = end - start;
        if (elapsed < 5000000)
        {
            count *= 2;
        }
    } while (elapsed < 5000000 && count <= 67108864);
    if (iterations != 0)
    {
        count = iterations;
        // 使用完整正式次数再预热一次，并保留结果，避免 Wasm 分层编译尚未稳定
        if (!exercise<Operation>(map, queries, count, checksum))
        {
            return false;
        }
        s_warmupChecksum = checksum;
    }
    (void)s_warmupChecksum;
    const uint64_t start = LockFreeMapBenchmarkClock::nanoseconds();
    if (!exercise<Operation>(map, queries, count, checksum))
    {
        return false;
    }
    const uint64_t end = LockFreeMapBenchmarkClock::nanoseconds();
    if (start == 0 || end <= start)
    {
        return false;
    }
    elapsed = end - start;
    uint64_t expectedSum = 0;
    for (uint64_t index = 0; index < count; ++index)
    {
        const uint32_t key = Operation == 0 ? static_cast<uint32_t>(index & (keys - 1)) * 2 :
            queries[static_cast<size_t>(index) & (keys - 1)];
        if (Operation <= 2)
        {
            expected[key / 2] = index + 1;
            expectedSum += index + 1;
        }
        else if (Operation == 4 || Operation == 6 || Operation == 13 || Operation == 16)
        {
            expectedSum += key + 17;
        }
        else if (Operation == 7 || Operation == 8)
        {
            expectedSum += 1;
        }
        else if (Operation == 9 || Operation == 10)
        {
            expectedSum += Operation == 9 ? 17 : (keys - 1) * 4 + 17;
        }
        else if (Operation == 11 || Operation == 12)
        {
            expectedSum += key == (keys - 1) * 2 ? 0 : (key + 2) * 2 + 17;
        }
        else if (Operation == 17)
        {
            expectedSum += uint64_t(keys) * (2 * (keys - 1) + 17);
        }
        else if (Operation >= 18 && Operation <= 20)
        {
            const uint64_t keySum = keys + (keys - 1) * 2;
            expectedSum += Operation == 18 ? keySum : (Operation == 19 ? keySum + 51 : keySum * 2 + 51);
        }
        else if (Operation >= 21)
        {
            expectedSum += keys;
        }
    }
    const bool valid = checksum == expectedSum && validate(map, expected);
    std::cout << name << ',' << keys << ',' << count << ',' << elapsed << ',' << checksum << ',' <<
        (valid ? "PASS" : "FAIL") << std::endl;
    return valid;
}