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
    /** 逐项核对键值及顺序，更新index和valid，不计入负载时间
    @param [in] key 当前受保护条目的键，仅回调期间借用
    @param [in] value 本次输入值；引用仅在当前回调期间借用，不向外保存
    */
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

bool LockFreeMapSingleBenchmark::sampleSparse(uint32_t keys)
{
    LockFreeMapSingleContainer map;
    std::vector<uint32_t> queries(keys);
    uint64_t expected = 0;
    for (uint32_t index = 0; index < keys; ++index)
    {
        // 奇数乘法在uint32_t中是一一映射，排序后间距不恒定；生成不计入时间。
        const uint32_t key = index * 2654435761U;
        queries[index] = key;
        expected += uint64_t(key) + 17;
        if (!insert(map, key))
        {
            return false;
        }
    }
    uint32_t random = 2463534242U;
    for (size_t index = queries.size() - 1; index > 0; --index)
    {
        random ^= random << 13;
        random ^= random >> 17;
        random ^= random << 5;
        std::swap(queries[index], queries[random % (index + 1)]);
    }
    const uint64_t iterations = 4194304;
    expected *= iterations / keys;
    uint64_t checksum = 0, elapsed = 0;
    for (unsigned round = 0; round < 3; ++round)
    {
        const uint64_t start = LockFreeMapBenchmarkClock::nanoseconds();
        if (!exercise<4>(map, queries, iterations, checksum) || checksum != expected)
        {
            return false;
        }
        const uint64_t end = LockFreeMapBenchmarkClock::nanoseconds();
        if (start == 0 || end <= start)
        {
            return false;
        }
        elapsed = end - start;
    }
    // 计时外逐键核对，不能以求和抵消两个错误值；前两轮为完整次数预热。
    for (size_t index = 0; index < queries.size(); ++index)
    {
        const uint32_t key = queries[index];
#ifdef LOCK_FREE_MAP_SINGLE_STD
        const LockFreeMapSingleContainer::const_iterator item = map.find(key);
        if (item == map.end() || item->second != uint64_t(key) + 17)
#else
        uint64_t value = 0;
        if (!map.get(key, value) || value != uint64_t(key) + 17)
#endif
        {
            return false;
        }
    }
    const bool valid = map.size() == keys;
    std::cout << "get_sparse," << keys << ',' << iterations << ',' << elapsed << ',' << checksum << ',' <<
        (valid ? "PASS" : "FAIL") << std::endl;
    return valid;
}

// 标准容器没有维护接口，本专项只比较当前Map与Git基线。
bool LockFreeMapSingleBenchmark::sampleIdle(bool maintenance)
{
#ifdef LOCK_FREE_MAP_SINGLE_STD
    (void)maintenance;
    return false;
#else
    LockFreeMapSingleContainer map;
    for (uint32_t key = 0; key < 128; ++key)
    {
        if (!insert(map, key))
        {
            return false;
        }
    }
    map.clear();
    size_t released = 0;
    if (map.lockFreeMapMaintain(4096, released) != LOCK_FREE_MAP_OK ||
        map.retainedNodeCount() != 0 || map.retainedValueCount() != 0)
    {
        return false;
    }
    const uint64_t iterations = maintenance ? UINT64_C(4194304) : UINT64_C(67108864);
    uint64_t elapsed = 0;
    for (unsigned round = 0; round < 3; ++round)
    {
        const uint64_t start = LockFreeMapBenchmarkClock::nanoseconds();
        for (uint64_t index = 0; index < iterations; ++index)
        {
            // 屏障与普通单线程基准一致；计时器仅在整段边界调用，不混入每次操作成本。
            barrier(&map);
            if (maintenance)
            {
                if (map.lockFreeMapMaintain(4096, released) != LOCK_FREE_MAP_OK)
                {
                    return false;
                }
            }
            else
            {
                map.clear();
            }
        }
        const uint64_t end = LockFreeMapBenchmarkClock::nanoseconds();
        if (start == 0 || end <= start || !map.empty() ||
            map.retainedNodeCount() != 0 || map.retainedValueCount() != 0)
        {
            return false;
        }
        elapsed = end - start;
    }
    std::cout << (maintenance ? "maintain_idle" : "clear_empty") << ",0," << iterations << ',' << elapsed << ",0,PASS" << std::endl;
    return std::cout.good();
#endif
}

int LockFreeMapSingleBenchmark::run(int argc, char** argv)
{
    if (argc == 2 && std::string(argv[1]) == "--selftest-output")
    {
        // 只破坏测试进程自己的输出流；真实执行一个最小场景，核对报告失败不会返回成功。
        std::string counts("1");
        for (size_t index = 1; index < 46; ++index)
        {
            counts += ",1";
        }
        char name[] = "LockFreeMapSingleBenchmark";
        char option[] = "--case";
        char number[] = "1";
        char* arguments[] = { name, const_cast<char*>(counts.c_str()), option, number };
        const std::ios::iostate saved = std::cout.rdstate();
        int result = 1;
        try
        {
            std::cout.setstate(std::ios::badbit);
            result = run(4, arguments);
        }
        catch (...)
        {
            std::cout.clear(saved);
            return 1;
        }
        std::cout.clear(saved);
        std::cout << "output_failure," << (result == 3 ? "PASS" : "FAIL") << std::endl;
        return result == 3 && std::cout.good() ? 0 : 1;
    }
    if (argc == 2 && std::string(argv[1]) == "--idle")
    {
        std::cout << "case,keys,iterations,nanoseconds,checksum,status" << std::endl;
        return sampleIdle(false) && sampleIdle(true) ? 0 : 3;
    }
    if (argc == 2 && std::string(argv[1]) == "--sparse")
    {
        std::cout << "case,keys,iterations,nanoseconds,checksum,status" << std::endl;
        return sampleSparse(128) && sampleSparse(4096) && std::cout.good() ? 0 : 3;
    }
    if (argc != 2 && argc != 4)
    {
        std::cerr << "usage: benchmark --selftest-output | --idle | --sparse | --calibrate | counts_csv [--case 1..46]" << std::endl;
        return 2;
    }
    size_t selected = 0;
    if (argc == 4)
    {
        std::istringstream selection(argv[3]);
        if (std::string(argv[2]) != "--case" || !(selection >> selected) || selected == 0 || selected > 46)
        {
            return 2;
        }
        selection >> std::ws;
        if (!selection.eof())
        {
            return 2;
        }
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
        if ((selected == 0 || selected == position + 1) && \
            !sample<number>(name, keys, calibrate ? 0 : counts[position])) { return 3; } \
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
    return std::cout.good() ? 0 : 3;
}

/** 执行单线程基准并把逃逸异常转换为进程退出码
@param [in] argc 参数数量
@param [in] argv 单项选择、校准、诊断或46项次数列表
@return 正常返回run的状态；标准异常返回100，其他异常返回101
*/
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