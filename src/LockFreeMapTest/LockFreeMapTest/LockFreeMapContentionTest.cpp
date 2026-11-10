#include "LockFreeMapTest.h"
#include "LockFreeMapProgressTest.h"
#include "LockFreeMapTestThread.h"
#include <limits>
#include <sstream>

int LockFreeMapContentionProtocol()
{
    static_assert(sizeof(LockFreeMapGuard) == sizeof(void*), "operation guard keeps one pointer");
    try
    {
        LockFreeMapReclamation domain(2);
        std::atomic<unsigned> completed(0);
        {
            // 竞争失败者暂停并持续占用一个记录，其他线程仍必须完成，不依赖其归还
            LockFreeMapGuard paused(domain);
            if (paused.m_record == nullptr)
            {
                return 1;
            }
            for (unsigned index = 0; index < 64; ++index)
            {
                paused.conflict();
            }
            // 请求者保持暂停，其他线程仍须消耗有限退避提示并持续推进
            domain.requestConditional();
            LockFreeMapTestThread worker([&domain, &completed]()
            {
                for (unsigned iteration = 0; iteration < 10000; ++iteration)
                {
                    {
                        LockFreeMapGuard guard(domain);
                        if (guard.m_record == nullptr)
                        {
                            return;
                        }
                    }
                    domain.finishConditional();
                    completed.fetch_add(1);
                }
            });
            if (!worker.join() || completed.load() != 10000 || domain.active() != 1)
            {
                return 1;
            }
            if (paused.m_record->m_contentionFailures != 0)
            {
                return 1;
            }
            {
                LockFreeMapGuard writer(domain);
                if (writer.m_record == nullptr)
                {
                    return 1;
                }
                for (unsigned attempt = 0; attempt < 7; ++attempt)
                {
                    writer.conflict(true);
                }
            }
            {
                LockFreeMapGuard reader(domain);
                if (reader.m_record == nullptr || reader.m_record->m_writeFailures != 7)
                {
                    return 1;
                }
                reader.conflict();
                if (reader.m_record->m_writeFailures != 7 || reader.m_record->m_contentionFailures != 1)
                {
                    return 1;
                }
                // 计数跨调用保留且读写独立；到固定阈值后归零，不能无限增长
                for (unsigned attempt = 7; attempt < 32; ++attempt)
                {
                    reader.conflict(true);
                }
                if (reader.m_record->m_writeFailures != 0 || reader.m_record->m_contentionFailures != 1)
                {
                    return 1;
                }
            }
        }
        if (domain.active() != 0)
        {
            return 1;
        }
        // 失败计数不具有资源所有权，暂停者退出后全部记录必须可以重新取得
        LockFreeMapGuard first(domain), second(domain), full(domain);
        if (first.m_record == nullptr || second.m_record == nullptr)
        {
            return 1;
        }
        // 独占写者合并统计须保留累计值，单记录及跨记录求和均不能回绕
        first.m_record->addValueRetries(7);
        first.m_record->addValueRetries(32);
        second.m_record->addValueRetries(3);
        if (domain.valueRetries() != 42)
        {
            return 1;
        }
        const uint64_t maximum = (std::numeric_limits<uint64_t>::max)();
        first.m_record->m_valueRetries.store(maximum - 10);
        first.m_record->addValueRetries(32);
        first.m_record->addValueRetries(1);
        if (first.m_record->m_valueRetries.load() != maximum || domain.valueRetries() != maximum)
        {
            return 1;
        }
        return first.m_record != nullptr && second.m_record != nullptr && full.m_record == nullptr ? 0 : 1;
    }
    catch (...)
    {
        return 1;
    }
}

int LockFreeMapConcurrentProgress()
{
    std::ostringstream report;
    const int result = LockFreeMapProgressTest::run(report, 200);
    const std::string text = report.str();
    if (LockFreeMapTestReport::m_file == nullptr ||
        std::fwrite(text.data(), 1, text.size(), LockFreeMapTestReport::m_file) != text.size())
    {
        return result + 1;
    }
    return result;
}

int LockFreeMapQueryProgress()
{
    std::ostringstream report;
    const unsigned changes = LockFreeMapProgressTest::queryChurn(report);
    if (changes == 0 || changes > 4)
    {
        return 1;
    }
    LockFreeMap<LockFreeMapProgressKey, int> map;
    if (!map.set(LockFreeMapProgressKey(5), 7))
    {
        return 1;
    }
    bool changed = false;
    unsigned errors = 0;
    // 查询已保护旧键后删除并重插，旧值仍可观察，但旧凭据不能修改新条目
    std::function<void()> replace = [&map, &changed, &errors]()
    {
        if (!changed)
        {
            changed = true;
            errors += map.erase(LockFreeMapProgressKey(5)) && map.set(LockFreeMapProgressKey(5), 99) ? 0 : 1;
        }
    };
    const LockFreeMapProgressKey query(5, &replace);
    int value = 0;
    LockFreeMapToken old;
    if (map.lockFreeMapGet(query, value, old) != LOCK_FREE_MAP_OK || value != 7 || !changed ||
        map.lockFreeMapUpdate(LockFreeMapProgressKey(5), old, 8) != LOCK_FREE_MAP_CONFLICT)
    {
        return 1;
    }
    changed = false;
    // clear 后新提交的键属于新索引；重叠查询允许完成旧观察，不能让旧凭据复活
    replace = [&map, &changed, &errors]()
    {
        if (!changed)
        {
            changed = true;
            map.clear();
            errors += map.set(LockFreeMapProgressKey(5), 101) ? 0 : 1;
        }
    };
    if (map.lockFreeMapGet(query, value, old) != LOCK_FREE_MAP_OK || value != 99 || !changed ||
        map.lockFreeMapErase(LockFreeMapProgressKey(5), old) != LOCK_FREE_MAP_CONFLICT)
    {
        return 1;
    }
    changed = false;
    // 条件提取会关闭旧值入口；随后重插时，旧视图的借用应返回未找到且不执行回调
    replace = [&map, &changed, &errors]()
    {
        if (!changed)
        {
            changed = true;
            int extracted = 0;
            errors += map.lockFreeMapExtract(LockFreeMapProgressKey(5), extracted) == LOCK_FREE_MAP_COMMITTED &&
                extracted == 101 && map.set(LockFreeMapProgressKey(5), 103) ? 0 : 1;
        }
    };
    unsigned calls = 0;
    // 关闭的旧入口不得执行用户借用回调
    const LockFreeMapStatus status = map.lockFreeMapVisit(query, [&calls](const int&)
    {
        ++calls;
    });
    if (status != LOCK_FREE_MAP_NOT_FOUND || calls != 0 || !changed ||
        !map.get(LockFreeMapProgressKey(5), value) || value != 103)
    {
        return 1;
    }
    unsigned invalidations = 0;
    // 每次比较都使已有根过期，强制生产和消费跨过多轮让步阈值后仍正常完成
    replace = [&map, &invalidations, &errors]()
    {
        if (invalidations < 256)
        {
            ++invalidations;
            errors += map.set(LockFreeMapProgressKey(100), 9) && map.erase(LockFreeMapProgressKey(100)) ? 0 : 1;
        }
    };
    if (!map.set(query, 107) || invalidations != 256 ||
        !map.get(LockFreeMapProgressKey(5), value) || value != 107)
    {
        return 1;
    }
    invalidations = 0;
    if (!map.erase(query) || invalidations != 256 ||
        map.lockFreeMapGet(LockFreeMapProgressKey(5), value, old) != LOCK_FREE_MAP_NOT_FOUND)
    {
        return 1;
    }
    map.clearAndFreeMemory();
    return errors == 0 && map.retainedNodeCount() == 0 && map.retainedValueCount() == 0 ? 0 : 1;
}