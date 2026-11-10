#include "LockFreeMapSingleBenchmark.h"
#include <exception>
#include <iostream>
#include <sstream>

bool LockFreeMapSingleBenchmark::insert(LockFreeMapSingleContainer& map, uint32_t key)
{
#ifdef LOCK_FREE_MAP_SINGLE_STD
    return map.insert(std::make_pair(key, uint64_t(key) + 17)).second;
#else
    return map.insert(key, uint64_t(key) + 17);
#endif
}

bool LockFreeMapSingleBenchmark::validate(const LockFreeMapSingleContainer& map,
    const std::vector<uint64_t>& expected)
{
    if (map.size() != expected.size() || map.empty() != expected.empty())
    {
        return false;
    }
    size_t index = 0;
    bool valid = true;
#ifdef LOCK_FREE_MAP_SINGLE_STD
    for (LockFreeMapSingleContainer::const_iterator item = map.begin(); item != map.end(); ++item)
    {
        valid = index < expected.size() && item->first == index * 2 && item->second == expected[index] && valid;
        ++index;
    }
#else
    // 核对每一个键值及顺序；失败只记录，不在回调中抛异常
    map.forEach([&index, &valid, &expected](const uint32_t& key, const uint64_t& value) -> void
    {
        valid = index < expected.size() && key == index * 2 && value == expected[index] && valid;
        ++index;
    });
    const std::vector<uint32_t> keys = map.keys();
    const std::vector<uint64_t> values = map.values();
    const std::vector<std::pair<uint32_t, uint64_t>> items = map.toVector();
    if (keys.size() != expected.size() || values.size() != expected.size() || items.size() != expected.size())
    {
        return false;
    }
    for (size_t item = 0; item < expected.size(); ++item)
    {
        valid = keys[item] == item * 2 && values[item] == expected[item] &&
            items[item].first == item * 2 && items[item].second == expected[item] && valid;
    }
#endif
    return valid && index == expected.size();
}

int LockFreeMapSingleBenchmark::run(int argc, char** argv)
{
    if (argc != 2)
    {
        std::cerr << "usage: benchmark --calibrate | counts_csv" << std::endl;
        return 2;
    }
    const bool calibrate = std::string(argv[1]) == "--calibrate";
    std::istringstream input(calibrate ? "" : argv[1]);
    std::vector<uint64_t> counts;
    if (!calibrate)
    {
        for (size_t index = 0; index < 46; ++index)
        {
            uint64_t count = 0;
            if (!(input >> count) || count == 0 || count > 268435456)
            {
                return 2;
            }
            counts.push_back(count);
            char separator = 0;
            if (index < 45 && (!(input >> separator) || separator != ','))
            {
                return 2;
            }
        }
        input >> std::ws;
        if (!input.eof())
        {
            return 2;
        }
    }
    std::cout << "case,keys,iterations,nanoseconds,checksum,status" << std::endl;
    size_t position = 0;
    for (uint32_t keys = 128; keys <= 4096; keys *= 32)
    {
        // 场景只在循环外分派；两个实现实例化相同的负载模板
#define LOCK_FREE_MAP_SINGLE_SAMPLE(number, name) \
        if (!sample<number>(name, keys, calibrate ? 0 : counts[position])) { return 3; } \
        ++position
        LOCK_FREE_MAP_SINGLE_SAMPLE(0, "set_sequence");
        LOCK_FREE_MAP_SINGLE_SAMPLE(1, "set_random");
        LOCK_FREE_MAP_SINGLE_SAMPLE(2, "subscript_assign");
        LOCK_FREE_MAP_SINGLE_SAMPLE(3, "insert_existing");
        LOCK_FREE_MAP_SINGLE_SAMPLE(4, "get_hit");
        LOCK_FREE_MAP_SINGLE_SAMPLE(5, "get_miss");
        LOCK_FREE_MAP_SINGLE_SAMPLE(6, "value_hit");
        LOCK_FREE_MAP_SINGLE_SAMPLE(7, "contains_hit");
        LOCK_FREE_MAP_SINGLE_SAMPLE(8, "count_hit");
        LOCK_FREE_MAP_SINGLE_SAMPLE(9, "first");
        LOCK_FREE_MAP_SINGLE_SAMPLE(10, "last");
        LOCK_FREE_MAP_SINGLE_SAMPLE(11, "lower_bound");
        LOCK_FREE_MAP_SINGLE_SAMPLE(12, "upper_bound");
        LOCK_FREE_MAP_SINGLE_SAMPLE(13, "visit_hit");
        LOCK_FREE_MAP_SINGLE_SAMPLE(14, "erase_miss");
        LOCK_FREE_MAP_SINGLE_SAMPLE(15, "erase_insert");
        LOCK_FREE_MAP_SINGLE_SAMPLE(16, "extract_insert");
        LOCK_FREE_MAP_SINGLE_SAMPLE(17, "for_each");
        LOCK_FREE_MAP_SINGLE_SAMPLE(18, "keys");
        LOCK_FREE_MAP_SINGLE_SAMPLE(19, "values");
        LOCK_FREE_MAP_SINGLE_SAMPLE(20, "to_vector");
        LOCK_FREE_MAP_SINGLE_SAMPLE(21, "insert_clear");
        LOCK_FREE_MAP_SINGLE_SAMPLE(22, "insert_free");
#undef LOCK_FREE_MAP_SINGLE_SAMPLE
    }
    return 0;
}

int main(int argc, char** argv)
{
    try
    {
        return LockFreeMapSingleBenchmark::run(argc, argv);
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
        return 100;
    }
    catch (...)
    {
        return 101;
    }
}