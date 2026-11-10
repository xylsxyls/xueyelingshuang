#pragma once
#include <algorithm>
#include <atomic>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#if defined(_MSC_VER)
#include <intrin.h>
#endif

template <typename V>
LockFreeHashMapSingleStd<V>::LockFreeHashMapSingleStd(size_t buckets) :
m_values(buckets)
{

}

template <typename V>
bool LockFreeHashMapSingleStd<V>::get(int32_t key, V& value) const
{
    typename std::unordered_map<int32_t, V>::const_iterator item = m_values.find(key);
    if (item == m_values.end())
    {
        return false;
    }
    value = item->second;
    return true;
}

template <typename V>
bool LockFreeHashMapSingleStd<V>::contains(int32_t key) const
{
    return m_values.find(key) != m_values.end();
}

template <typename V>
size_t LockFreeHashMapSingleStd<V>::count(int32_t key) const
{
    return m_values.count(key);
}

template <typename V>
V LockFreeHashMapSingleStd<V>::value(int32_t key) const
{
    typename std::unordered_map<int32_t, V>::const_iterator item = m_values.find(key);
    return item == m_values.end() ? V() : item->second;
}

template <typename V>
V& LockFreeHashMapSingleStd<V>::operator[](int32_t key)
{
    return m_values[key];
}

template <typename V>
bool LockFreeHashMapSingleStd<V>::set(int32_t key, const V& value)
{
    m_values[key] = value;
    return true;
}

template <typename V>
bool LockFreeHashMapSingleStd<V>::insert(int32_t key, const V& value)
{
    return m_values.emplace(key, value).second;
}

template <typename V>
bool LockFreeHashMapSingleStd<V>::erase(int32_t key)
{
    return m_values.erase(key) != 0;
}

template <typename V>
size_t LockFreeHashMapSingleStd<V>::size() const
{
    return m_values.size();
}

template <typename V>
bool LockFreeHashMapSingleStd<V>::empty() const
{
    return m_values.empty();
}

template <typename V>
size_t LockFreeHashMapSingleStd<V>::bucketCount() const
{
    return m_values.bucket_count();
}

template <typename V>
void LockFreeHashMapSingleStd<V>::clear()
{
    m_values.clear();
}

template <typename V>
template <typename F>
size_t LockFreeHashMapSingleStd<V>::forEach(const F& function) const
{
    size_t visited = 0;
    for (typename std::unordered_map<int32_t, V>::const_iterator item = m_values.begin(); item != m_values.end(); ++item)
    {
        function(item->first, item->second);
        ++visited;
    }
    return visited;
}

template <typename V>
std::vector<int32_t> LockFreeHashMapSingleStd<V>::keys() const
{
    std::vector<int32_t> output;
    output.reserve(m_values.size());
    for (typename std::unordered_map<int32_t, V>::const_iterator item = m_values.begin(); item != m_values.end(); ++item)
    {
        output.push_back(item->first);
    }
    return output;
}

template <typename V>
std::vector<V> LockFreeHashMapSingleStd<V>::values() const
{
    std::vector<V> output;
    output.reserve(m_values.size());
    for (typename std::unordered_map<int32_t, V>::const_iterator item = m_values.begin(); item != m_values.end(); ++item)
    {
        output.push_back(item->second);
    }
    return output;
}

template <typename V>
std::vector<std::pair<int32_t, V>> LockFreeHashMapSingleStd<V>::toVector() const
{
    std::vector<std::pair<int32_t, V>> output;
    output.reserve(m_values.size());
    for (typename std::unordered_map<int32_t, V>::const_iterator item = m_values.begin(); item != m_values.end(); ++item)
    {
        output.emplace_back(item->first, item->second);
    }
    return output;
}

template <typename V>
V LockFreeHashMapSingleData<V>::make(int32_t marker)
{
    return V(marker);
}

template <typename V>
uint64_t LockFreeHashMapSingleData<V>::marker(const V& value)
{
    return static_cast<uint64_t>(value);
}

template <typename V>
bool LockFreeHashMapSingleData<V>::released(const V& value)
{
    (void)value;
    return true;
}

inline void LockFreeHashMapSingleThread::compilerBarrier()
{
#if defined(_MSC_VER)
    _ReadWriteBarrier();
#else
    std::atomic_signal_fence(std::memory_order_seq_cst);
#endif
}

template <typename V>
void LockFreeHashMapSingleThread::create(size_t count, std::unique_ptr<LockFreeHashMapSingleStd<V>>& map)
{
    map.reset(new LockFreeHashMapSingleStd<V>(count));
}

template <typename V>
void LockFreeHashMapSingleThread::create(size_t count, std::unique_ptr<LockFreeHashMap<int32_t, V>>& map)
{
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = count * 2 + 128;
    map.reset(new LockFreeHashMap<int32_t, V>(count, config));
    if (map->status() != LockFreeHashMapOk)
    {
        throw std::runtime_error("本库初始化失败");
    }
}

template <typename V>
void LockFreeHashMapSingleThread::reclaim(LockFreeHashMapSingleStd<V>& map)
{
    if (!map.empty())
    {
        throw std::runtime_error("标准表清空失败");
    }
}

template <typename V>
void LockFreeHashMapSingleThread::reclaim(LockFreeHashMap<int32_t, V>& map)
{
    for (size_t attempt = 0; attempt < 64; ++attempt)
    {
        if (map.maintain(map.bucketCount() + 8192).m_status != LockFreeHashMapOk)
        {
            throw std::runtime_error("本库维护失败");
        }
        LockFreeHashMapPoolStats stats;
        if (map.poolStats(stats).m_status != LockFreeHashMapOk)
        {
            throw std::runtime_error("本库资源查询失败");
        }
        if (stats.occupied == 0 && stats.retired == 0)
        {
            return;
        }
    }
    throw std::runtime_error("本库节点未完成回收");
}

template <typename Map, typename V, int32_t Operation>
LockFreeHashMapSingleSample LockFreeHashMapSingleThread::batch(const std::vector<int32_t>& order,
    const std::vector<V>& values, size_t loops)
{
    const size_t count = order.size();
    const uint64_t valueSum = uint64_t(count) * (count + 1) / 2;
    const uint64_t keySum = uint64_t(count) * (count - 1) / 2;
    LockFreeHashMapSingleSample result;
    if (Operation == SingleClear || Operation == SingleClearReclaim)
    {
        // 同时准备六十四张表，时钟只包住整批操作，降低极短 clear 的时钟量化影响。
        std::vector<std::unique_ptr<Map>> maps(64);
        for (size_t i = 0; i < maps.size(); ++i)
        {
            create(count, maps[i]);
            for (size_t j = 0; j < count; ++j)
            {
                if (!maps[i]->insert(order[j], values[order[j]]))
                {
                    throw std::runtime_error("清空夹具填充失败");
                }
            }
        }
        result.m_bucketsBefore = maps[0]->bucketCount();
        double begin = timestamp();
        for (size_t i = 0; i < maps.size(); ++i)
        {
            maps[i]->clear();
            if (Operation == SingleClearReclaim)
            {
                reclaim(*maps[i]);
            }
        }
        result.m_seconds = timestamp() - begin;
        result.m_calls = maps.size();
        for (size_t i = 0; i < maps.size(); ++i)
        {
            result.m_errors += !maps[i]->empty();
            reclaim(*maps[i]);
        }
        result.m_bucketsAfter = maps[0]->bucketCount();
        return result;
    }
    if (Operation == SingleLifecycle)
    {
        double begin = timestamp();
        for (size_t loop = 0; loop < loops; ++loop)
        {
            std::unique_ptr<Map> map;
            create(count, map);
            for (size_t i = 0; i < count; ++i)
            {
                result.m_errors += !map->insert(order[i], values[order[i]]);
            }
            result.m_checksum += map->size();
            result.m_bucketsBefore = count;
            result.m_bucketsAfter = map->bucketCount();
        }
        result.m_seconds = timestamp() - begin;
        result.m_calls = loops;
        result.m_errors += result.m_checksum != uint64_t(count) * loops;
        return result;
    }
    std::unique_ptr<Map> holder;
    create(count, holder);
    Map& map = *holder;
    if (Operation != SingleInsertNew)
    {
        for (size_t i = 0; i < count; ++i)
        {
            if (!map.insert(order[i], values[order[i]]))
            {
                throw std::runtime_error("夹具填充失败");
            }
        }
    }
    result.m_bucketsBefore = map.bucketCount();
    V output = V();
    std::vector<int32_t> exportedKeys;
    std::vector<V> exportedValues;
    std::vector<std::pair<int32_t, V>> exportedPairs;
    const bool whole = Operation >= SingleTraverse;
    const size_t calls = whole ? loops : count * loops;
    double begin = timestamp();
    for (size_t i = 0; i < calls; ++i)
    {
        // 两角色共用编译器屏障；纯数量接口另外通过禁止跨单元优化的函数边界调用。
        compilerBarrier();
        int32_t key = order[i & (count - 1)];
        if (Operation == SingleGetHit || Operation == SingleGetMiss)
        {
            bool found = map.get(Operation == SingleGetMiss ? key + static_cast<int32_t>(count) : key, output);
            result.m_errors += found != (Operation == SingleGetHit);
            if (Operation == SingleGetHit)
            {
                result.m_checksum += LockFreeHashMapSingleData<V>::marker(output);
            }
        }
        else if (Operation == SingleContainsHit || Operation == SingleContainsMiss)
        {
            result.m_checksum += map.contains(Operation == SingleContainsMiss ? key + static_cast<int32_t>(count) : key);
        }
        else if (Operation == SingleCountKey)
        {
            result.m_checksum += map.count(key);
        }
        else if (Operation == SingleValue || Operation == SingleSubscriptRead)
        {
            output = Operation == SingleValue ? map.value(key) : V(map[key]);
            result.m_checksum += LockFreeHashMapSingleData<V>::marker(output);
        }
        else if (Operation == SingleSet)
        {
            result.m_errors += !map.set(key, values[count + key]);
        }
        else if (Operation == SingleSubscriptWrite)
        {
            map[key] = values[count + key];
        }
        else if (Operation == SingleInsertNew || Operation == SingleInsertExists)
        {
            result.m_errors += map.insert(key, values[key]) != (Operation == SingleInsertNew);
        }
        else if (Operation == SingleEraseHit || Operation == SingleEraseMiss)
        {
            result.m_errors += map.erase(Operation == SingleEraseMiss ? key + static_cast<int32_t>(count) : key) != (Operation == SingleEraseHit);
        }
        else if (Operation == SingleChurn)
        {
            result.m_errors += !map.erase(key);
            result.m_errors += !map.insert(key, values[key]);
        }
        else if (Operation == SingleMixed)
        {
            if (i % 10 == 9)
            {
                result.m_errors += !map.set(key, values[count + key]);
            }
            else
            {
                result.m_errors += !map.get(key, output);
                result.m_checksum += LockFreeHashMapSingleData<V>::marker(output);
            }
        }
        else if (Operation == SingleSize)
        {
            result.m_checksum += LockFreeHashMapSingleObservation::size(map);
        }
        else if (Operation == SingleEmpty)
        {
            result.m_checksum += LockFreeHashMapSingleObservation::empty(map);
        }
        else if (Operation == SingleTraverse)
        {
            // 借用当次键值并累加，两个容器承担相同的回调工作。
            size_t visited = map.forEach([&result](const int32_t& entryKey, const V& entryValue)
            {
                result.m_checksum += static_cast<uint64_t>(entryKey) + LockFreeHashMapSingleData<V>::marker(entryValue);
            });
            result.m_errors += visited != count;
        }
        else if (Operation == SingleKeys)
        {
            exportedKeys = map.keys();
            result.m_errors += exportedKeys.size() != count;
        }
        else if (Operation == SingleValues)
        {
            exportedValues = map.values();
            result.m_errors += exportedValues.size() != count;
        }
        else if (Operation == SinglePairs)
        {
            exportedPairs = map.toVector();
            result.m_errors += exportedPairs.size() != count;
        }
    }
    result.m_seconds = timestamp() - begin;
    result.m_calls = calls;
    result.m_bucketsAfter = map.bucketCount();
    uint64_t expected = 0;
    if (Operation == SingleGetHit || Operation == SingleValue || Operation == SingleSubscriptRead)
    {
        expected = valueSum * loops;
    }
    else if (Operation == SingleContainsHit || Operation == SingleCountKey)
    {
        expected = calls;
    }
    else if (Operation == SingleSize)
    {
        expected = uint64_t(count) * calls;
    }
    else if (Operation == SingleTraverse)
    {
        expected = (keySum + valueSum) * loops;
    }
    std::vector<unsigned char> states(count, 0);
    if (Operation == SingleMixed)
    {
        // 独立数组模型在计时外重放，不以另一容器的输出当作正确答案。
        for (size_t i = 0; i < calls; ++i)
        {
            size_t key = order[i & (count - 1)];
            if (i % 10 == 9)
            {
                states[key] = 1;
            }
            else
            {
                expected += key + 1 + states[key] * count;
            }
        }
    }
    if (Operation == SingleKeys)
    {
        std::sort(exportedKeys.begin(), exportedKeys.end());
        for (size_t i = 0; i < exportedKeys.size(); ++i)
        {
            result.m_errors += exportedKeys[i] != static_cast<int32_t>(i);
        }
    }
    if (Operation == SingleValues)
    {
        std::vector<uint64_t> markers;
        for (size_t i = 0; i < exportedValues.size(); ++i)
        {
            markers.push_back(LockFreeHashMapSingleData<V>::marker(exportedValues[i]));
        }
        std::sort(markers.begin(), markers.end());
        for (size_t i = 0; i < markers.size(); ++i)
        {
            result.m_errors += markers[i] != i + 1;
        }
    }
    if (Operation == SinglePairs)
    {
        std::vector<unsigned char> seen(count, 0);
        for (size_t i = 0; i < exportedPairs.size(); ++i)
        {
            int32_t key = exportedPairs[i].first;
            if (key < 0 || static_cast<size_t>(key) >= count)
            {
                ++result.m_errors;
                continue;
            }
            result.m_errors += seen[key] != 0;
            seen[key] = 1;
            result.m_errors += LockFreeHashMapSingleData<V>::marker(exportedPairs[i].second) != uint64_t(key + 1);
        }
    }
    result.m_errors += result.m_checksum != expected;
    result.m_errors += map.size() != (Operation == SingleEraseHit ? 0 : count);
    for (size_t i = 0; i < count; ++i)
    {
        bool found = map.get(static_cast<int32_t>(i), output);
        result.m_errors += found != (Operation != SingleEraseHit);
        if (found)
        {
            uint64_t marker = i + 1;
            if (Operation == SingleSet || Operation == SingleSubscriptWrite || (Operation == SingleMixed && states[i]))
            {
                marker += count;
            }
            result.m_errors += LockFreeHashMapSingleData<V>::marker(output) != marker;
        }
    }
    map.clear();
    reclaim(map);
    return result;
}

template <typename V, int32_t Operation>
bool LockFreeHashMapSingleThread::compare(const char* name, const std::vector<int32_t>& order,
    const std::vector<V>& values, size_t rounds, double milliseconds, std::ofstream& raw, std::ofstream& summary)
{
    typedef LockFreeHashMap<int32_t, V> Current;
    typedef LockFreeHashMapSingleStd<V> Standard;
    size_t loops[2] = { 1, 1 };
    const bool fixed = Operation == SingleInsertNew || Operation == SingleEraseHit || Operation == SingleClear || Operation == SingleClearReclaim;
    // 预热也核对返回与结果；破坏性单次操作不为了凑时长反复清空准备，原始记录保留实际时长。
    for (size_t role = 0; role < 2; ++role)
    {
        for (;;)
        {
            LockFreeHashMapSingleSample sample = role == 0 ? batch<Current, V, Operation>(order, values, loops[role]) :
                batch<Standard, V, Operation>(order, values, loops[role]);
            if (sample.m_errors || sample.m_seconds <= 0)
            {
                throw std::runtime_error("预热或校准结果不正确");
            }
            if (fixed || sample.m_seconds * 1000 >= milliseconds || loops[role] >= 1048576)
            {
                break;
            }
            loops[role] *= 2;
        }
    }
    std::vector<double> current;
    std::vector<double> standard;
    std::vector<double> paired;
    for (size_t round = 0; round < rounds; ++round)
    {
        double nanos[2] = {};
        for (size_t slot = 0; slot < 2; ++slot)
        {
            size_t role = (round + slot + static_cast<size_t>(Operation)) % 2;
            LockFreeHashMapSingleSample sample = role == 0 ? batch<Current, V, Operation>(order, values, loops[role]) :
                batch<Standard, V, Operation>(order, values, loops[role]);
            if (sample.m_errors || sample.m_seconds <= 0 || sample.m_calls == 0)
            {
                std::cerr << "FAILED " << name << " " << order.size() << " " << operationName(Operation)
                    << " role=" << role << " errors=" << sample.m_errors << std::endl;
                return false;
            }
            nanos[role] = sample.m_seconds * 1e9 / double(sample.m_calls);
            raw << name << ',' << order.size() << ',' << Operation << ',' << operationName(Operation) << ','
                << round + 1 << ',' << slot + 1 << ',' << (role == 0 ? "current" : "std_unlocked") << ','
                << loops[role] << ',' << sample.m_calls << ',' << sample.m_seconds << ',' << nanos[role] << ','
                << sample.m_checksum << ',' << sample.m_errors << ',' << sample.m_bucketsBefore << ',' << sample.m_bucketsAfter << '\n';
        }
        current.push_back(nanos[0]);
        standard.push_back(nanos[1]);
        paired.push_back((nanos[1] / nanos[0] - 1) * 100);
    }
    std::sort(current.begin(), current.end());
    std::sort(standard.begin(), standard.end());
    std::sort(paired.begin(), paired.end());
    double currentMedian = current[rounds / 2];
    double standardMedian = standard[rounds / 2];
    summary << name << ',' << order.size() << ',' << Operation << ',' << operationName(Operation) << ',' << rounds << ','
        << currentMedian << ',' << standardMedian << ',' << (standardMedian / currentMedian - 1) * 100 << ','
        << 1e9 / currentMedian << ',' << 1e9 / standardMedian << ',' << current.front() << ',' << current.back() << ','
        << standard.front() << ',' << standard.back() << ',' << paired[rounds / 2] << '\n';
    raw.flush();
    summary.flush();
    for (size_t i = 0; i < values.size(); ++i)
    {
        if (!LockFreeHashMapSingleData<V>::released(values[i]))
        {
            throw std::runtime_error("候选以外的共享拥有引用未释放");
        }
    }
    std::cout << name << " keys=" << order.size() << " " << operationName(Operation)
        << " current_ns=" << currentMedian << " std_ns=" << standardMedian << " throughput_change_pct="
        << (standardMedian / currentMedian - 1) * 100 << std::endl;
    return raw.good() && summary.good();
}

template <typename V>
bool LockFreeHashMapSingleThread::suite(const char* name, size_t count, size_t rounds, double milliseconds,
    int32_t first, int32_t last, std::ofstream& raw, std::ofstream& summary)
{
    std::vector<int32_t> order;
    std::vector<V> values;
    order.reserve(count);
    values.reserve(count * 2);
    for (size_t i = 0; i < count; ++i)
    {
        order.push_back(static_cast<int32_t>(i));
    }
    for (size_t i = 0; i < count * 2; ++i)
    {
        values.push_back(LockFreeHashMapSingleData<V>::make(static_cast<int32_t>(i + 1)));
    }
    uint32_t random = UINT32_C(0x1234abcd);
    for (size_t i = count - 1; i > 0; --i)
    {
        random = random * 1664525U + 1013904223U;
        std::swap(order[i], order[static_cast<size_t>(random) % (i + 1)]);
    }
    // 操作在计时外分派，模板常量使循环不携带运行期 switch 的分支成本。
#define LOCKFREEHASHMAP_SINGLE_CASE(operation) \
    if (first <= operation && last >= operation && !compare<V, operation>(name, order, values, rounds, milliseconds, raw, summary)) \
    { \
        return false; \
    }
    LOCKFREEHASHMAP_SINGLE_CASE(SingleGetHit)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleGetMiss)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleContainsHit)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleContainsMiss)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleCountKey)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleValue)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleSubscriptRead)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleSet)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleSubscriptWrite)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleInsertNew)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleInsertExists)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleEraseHit)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleEraseMiss)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleChurn)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleMixed)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleSize)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleEmpty)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleTraverse)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleKeys)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleValues)
    LOCKFREEHASHMAP_SINGLE_CASE(SinglePairs)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleClear)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleClearReclaim)
    LOCKFREEHASHMAP_SINGLE_CASE(SingleLifecycle)
#undef LOCKFREEHASHMAP_SINGLE_CASE
    return true;
}