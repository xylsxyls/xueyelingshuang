#include "LockFreeMapTest.h"
#include "LockFreeMap/LockFreeMapAPI.h"
#include <map>
#include <memory>
#include <string>

int LockFreeMapCompileExamples();

/** 遍历回调中生成覆盖视图，再清空并维护；两层旧视图仍须完整有序
@return 回调键值匹配独立固定序列，新根只含后插入项时true
*/
static bool CheckTraversalReentry()
{
    LockFreeMap<int, int> map;
    if (!map.set(0, 0) || !map.set(2, 20) || !map.set(4, 40))
    {
        return false;
    }
    const int outerKeys[] = { 0, 2, 4 };
    const int innerKeys[] = { 0, 1, 4 };
    size_t outerSeen = 0, innerSeen = 0, innerCompleted = 0;
    bool valid = true;
    // 参数为外层保护视图中的借用，回调重入后仍只核对旧视图的固定序列。
    const size_t outerCompleted = map.forEach([&](const int& key, const int& value)
    {
        valid = outerSeen < 3 && key == outerKeys[outerSeen] && value == key * 10 && valid;
        if (++outerSeen == 1)
        {
            valid = map.erase(2) && map.set(1, 10) && valid;
            // 参数为内层覆盖视图中的借用；清空、重插和回收不能更改本次键集合。
            innerCompleted = map.forEach([&](const int& innerKey, const int& innerValue)
            {
                valid = innerSeen < 3 && innerKey == innerKeys[innerSeen] && innerValue == innerKey * 10 && valid;
                if (++innerSeen == 1)
                {
                    map.clear();
                    valid = map.set(9, 90) && valid;
                    size_t released = 0;
                    valid = map.lockFreeMapMaintain(4096, released) == LOCK_FREE_MAP_OK && valid;
                }
            });
        }
    });
    int value = 0;
    return valid && outerCompleted == 3 && outerSeen == 3 && innerCompleted == 3 && innerSeen == 3 &&
        map.size() == 1 && map.get(9, value) && value == 90;
}

/** 保持旧遍历时向数组尾部追加，再建立新视图并继续追加与清空
@return 两层遍历仍按各自固定键序列返回，后续新根和回收均正确时true
*/
static bool CheckTraversalAppend()
{
    LockFreeMap<int, int> map;
    for (int key = 0; key < 3; ++key)
    {
        if (!map.set(key, key + 10))
        {
            return false;
        }
    }
    size_t outerSeen = 0, innerSeen = 0, innerCompleted = 0;
    bool valid = true;
    // 旧视图不包含之后追加的键；持有它时内层新视图必须能读取新尾项。
    const size_t outerCompleted = map.forEach([&](const int& key, const int& value)
    {
        valid = key == static_cast<int>(outerSeen) && value == key + 10 && valid;
        if (++outerSeen == 1)
        {
            valid = map.set(3, 13) && valid;
            // 同时保留两层视图后继续追加，检查新索引不依赖旧视图及时释放。
            innerCompleted = map.forEach([&](const int& innerKey, const int& innerValue)
            {
                valid = innerKey == static_cast<int>(innerSeen) && innerValue == innerKey + 10 && valid;
                if (++innerSeen == 1)
                {
                    valid = map.set(4, 14) && valid;
                    map.clear();
                    valid = map.set(9, 19) && valid;
                    size_t released = 0;
                    valid = map.lockFreeMapMaintain(4096, released) == LOCK_FREE_MAP_OK && valid;
                }
            });
        }
    });
    int value = 0;
    return valid && outerCompleted == 3 && outerSeen == 3 && innerCompleted == 4 && innerSeen == 4 &&
        map.size() == 1 && map.get(9, value) && value == 19;
}

/** 整池释放必须销毁非平凡键和值，重复使用同一实例不遗留拥有引用
@return 取消外部拥有者后资源仍被容器保有，释放后弱引用全部过期时true
*/
static bool CheckWholeStoreOwnership()
{
    LockFreeMap<std::shared_ptr<int>, int> keys;
    LockFreeMap<int, std::shared_ptr<int>> values;
    LockFreeMap<std::shared_ptr<int>, std::shared_ptr<int>> both;
    for (int round = 0; round < 3; ++round)
    {
        std::shared_ptr<int> key(new int(71)), value(new int(93));
        const std::weak_ptr<int> weakKey(key), weakValue(value);
        if (!keys.set(key, 17) || !values.set(1, value) || !both.set(key, value))
        {
            return false;
        }
        key.reset();
        value.reset();
        if (weakKey.expired() || weakValue.expired())
        {
            return false;
        }
        keys.clearAndFreeMemory();
        values.clearAndFreeMemory();
        if (weakKey.expired() || weakValue.expired())
        {
            return false;
        }
        both.clearAndFreeMemory();
        if (!weakKey.expired() || !weakValue.expired() || !keys.empty() || !values.empty() || !both.empty())
        {
            return false;
        }
    }
    return true;
}

/** 以独立有序容器对照公开操作
@return 失败时返回非零
*/
int LockFreeMapBasic()
{
    int failed = 0;
    try
    {
        const bool traversal = CheckTraversalReentry();
        const bool ownership = CheckWholeStoreOwnership();
        const bool appended = CheckTraversalAppend();
        if (!traversal || !ownership || !appended)
        {
            std::fprintf(LockFreeMapTestReport::m_file, "basic_failure,stage=lifetime,traversal=%d,ownership=%d,append=%d\n", traversal, ownership, appended);
            return 1;
        }
        // 延迟构造在配置前不建立池；失败配置不能使实例进入半初始化状态。
        LockFreeMap<int, int> deferred(false);
        LockFreeMapToken unused;
        int unchanged = 97;
        failed += !deferred.empty() || deferred.retainedNodeCount() != 0 || deferred.retainedValueCount() != 0;
        failed += deferred.lockFreeMapGet(1, unchanged, unused) != LOCK_FREE_MAP_INVALID || unchanged != 97;
        failed += deferred.lockFreeMapSet(1, 2) != LOCK_FREE_MAP_INVALID;
        LockFreeMapConfig deferredConfig;
        deferredConfig.operationRecordLimit = 0;
        failed += deferred.lockFreeMapConfigure(deferredConfig) != LOCK_FREE_MAP_INVALID;
        deferredConfig.operationRecordLimit = 8;
        deferredConfig.nodes.initialCapacity = 2;
        deferredConfig.values.initialCapacity = 2;
        failed += deferred.lockFreeMapConfigure(deferredConfig) != LOCK_FREE_MAP_OK;
        failed += !deferred.set(1, 2) || !deferred.get(1, unchanged) || unchanged != 2;
        deferred.clearAndFreeMemory();
        failed += !deferred.set(3, 4) || !deferred.get(3, unchanged) || unchanged != 4;
        LockFreeMap<int, int> immediate(true);
        failed += !immediate.set(5, 6) || !immediate.get(5, unchanged) || unchanged != 6;
        if (failed != 0)
        {
            std::fprintf(LockFreeMapTestReport::m_file, "basic_failure,stage=deferred_construction,result=%d\n", failed);
        }
        const int example = LockFreeMapCompileExamples();
        failed += example != 0;
        if (example != 0)
        {
            std::fprintf(LockFreeMapTestReport::m_file, "basic_failure,stage=examples,result=%d\n", example);
        }
        LockFreeMap<int, int> actual;
        std::map<int, int> expected;
        uint32_t seed = 271828;
        for (size_t step = 0; step < 20000; ++step)
        {
            seed = seed * 1664525u + 1013904223u;
            const int key = static_cast<int>((seed >> 8) % 63);
            const int value = static_cast<int>(seed >> 1);
            const unsigned action = seed % 7;
            if (action < 2)
            {
                failed += !actual.set(key, value);
                expected[key] = value;
            }
            else if (action == 2)
            {
                failed += actual.insert(key, value) != expected.insert(std::make_pair(key, value)).second;
            }
            else if (action == 3)
            {
                failed += actual.erase(key) != (expected.erase(key) != 0);
            }
            else if (action == 4 && step % 131 == 0)
            {
                actual.clear();
                expected.clear();
            }
            else
            {
                int foundKey = -1;
                int foundValue = -1;
                std::map<int, int>::iterator bound = action == 5 ? expected.lower_bound(key) : expected.upper_bound(key);
                bool found = action == 5 ? actual.lowerBound(key, foundKey, foundValue) : actual.upperBound(key, foundKey, foundValue);
                failed += found != (bound != expected.end());
                if (found && bound != expected.end())
                {
                    failed += foundKey != bound->first || foundValue != bound->second;
                }
            }
            const int operationFailures = failed;
            std::vector<std::pair<int, int>> snapshot = actual.toVector();
            failed += snapshot.size() != expected.size();
            size_t index = 0;
            for (std::map<int, int>::iterator entry = expected.begin(); entry != expected.end() && index < snapshot.size(); ++entry, ++index)
            {
                failed += snapshot[index].first != entry->first || snapshot[index].second != entry->second;
            }
            if (failed != 0)
            {
                // 保存首个偏差的输入和阶段，不能把随机对照失败只记成一个布尔值
                std::fprintf(LockFreeMapTestReport::m_file,
                    "basic_failure,stage=random,step=%llu,action=%u,key=%d,operation_failures=%d,total_failures=%d,actual=%llu,expected=%llu\n",
                    static_cast<unsigned long long>(step), action, key, operationFailures, failed,
                    static_cast<unsigned long long>(snapshot.size()), static_cast<unsigned long long>(expected.size()));
                break;
            }
        }
        const int beforeTokens = failed;
        actual.clearAndFreeMemory();
        failed += !actual.empty() || actual.retainedNodeCount() != 0 || actual.retainedValueCount() != 0;
        failed += !actual.set(1, 10);
        int value = 0;
        LockFreeMapToken stale;
        failed += actual.lockFreeMapGet(1, value, stale) != LOCK_FREE_MAP_OK;
        failed += actual.lockFreeMapUpdate(1, stale, 11) != LOCK_FREE_MAP_COMMITTED;
        failed += actual.lockFreeMapErase(1, stale) != LOCK_FREE_MAP_CONFLICT;
        LockFreeMapToken current;
        failed += actual.lockFreeMapGet(1, value, current) != LOCK_FREE_MAP_OK || value != 11;
        failed += actual.lockFreeMapErase(1, current) != LOCK_FREE_MAP_COMMITTED;
        failed += !actual.set(1, 10);
        failed += actual.lockFreeMapErase(1, stale) != LOCK_FREE_MAP_CONFLICT;
        actual.clear();
        failed += !actual.set(1, 10);
        failed += actual.lockFreeMapErase(1, current) != LOCK_FREE_MAP_CONFLICT;
        LockFreeMap<int, int> other;
        other.set(1, 11);
        failed += other.lockFreeMapErase(1, current) != LOCK_FREE_MAP_CONFLICT;
        failed += actual.lockFreeMapExtract(1, value) != LOCK_FREE_MAP_COMMITTED || value != 10;
        failed += actual.lockFreeMapExtract(1, value) != LOCK_FREE_MAP_NOT_FOUND;
        if (failed != beforeTokens)
        {
            std::fprintf(LockFreeMapTestReport::m_file, "basic_failure,stage=tokens,failures=%d\n", failed - beforeTokens);
        }
    }
    catch (...)
    {
        std::fprintf(LockFreeMapTestReport::m_file, "basic_failure,stage=exception\n");
        ++failed;
    }

    return failed == 0 ? 0 : 1;
}