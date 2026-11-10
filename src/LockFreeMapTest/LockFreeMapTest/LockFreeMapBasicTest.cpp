#include "LockFreeMapTest.h"
#include "LockFreeMap/LockFreeMapAPI.h"
#include <map>
#include <string>

int LockFreeMapCompileExamples();

/** 以独立有序容器对照公开操作
@return 失败时返回非零
*/
int LockFreeMapBasic()
{
    int failed = 0;
    try
    {
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