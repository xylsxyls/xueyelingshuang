#include "LockFreeMapProgressTest.h"
#include "LockFreeMapBenchmarkClock.h"
#include "LockFreeMapTestThread.h"
#include <algorithm>
#include <atomic>
#include <thread>

LockFreeMapProgressKey::LockFreeMapProgressKey(int value, std::function<void()>* compare) :
m_value(value),
m_compare(compare)
{

}

bool LockFreeMapProgressKey::operator<(const LockFreeMapProgressKey& other) const
{
    std::function<void()>* function = m_compare != nullptr ? m_compare : other.m_compare;
    if (function != nullptr)
    {
        (*function)();
    }
    return m_value < other.m_value;
}

unsigned LockFreeMapProgressTest::queryChurn(std::ostream& report)
{
#ifdef LOCK_FREE_MAP_PROGRESS_COMMON
    // 线上源码没有借用接口，共有接口性能入口不提供该专用诊断
    (void)report;
    return 0;
#else
    LockFreeMap<LockFreeMapProgressKey, int> map;
    if (!map.set(LockFreeMapProgressKey(5), 7))
    {
        return 0;
    }
    unsigned changes = 0, errors = 0, maximum = 0;
    // 比较器排序仍只由整数决定；每次比较通过另一个独立调用改变无关键
    std::function<void()> change = [&map, &changes, &errors]()
    {
        if (changes < 32)
        {
            ++changes;
            errors += map.set(LockFreeMapProgressKey(100), 9) && map.erase(LockFreeMapProgressKey(100)) ? 0 : 1;
        }
    };
    const LockFreeMapProgressKey query(5, &change);
    int value = 0;
    LockFreeMapToken token;
    if (map.lockFreeMapGet(query, value, token) != LOCK_FREE_MAP_OK || value != 7 || changes == 0)
    {
        return 0;
    }
    maximum = changes;
    report << "query_churn,get," << changes << '\n';
    changes = 0;
    unsigned calls = 0;
    // 回调只读取本次借用值，不执行额外结构修改
    const LockFreeMapStatus status = map.lockFreeMapVisit(query, [&value, &calls](const int& borrowed)
    {
        value = borrowed;
        ++calls;
    });
    report << "query_churn,visit," << changes << '\n';
    return errors == 0 && status == LOCK_FREE_MAP_OK && value == 7 && calls == 1 && changes != 0 ?
        (std::max)(maximum, changes) : 0;
#endif
}

LockFreeMapProgressWorker::LockFreeMapProgressWorker() :
m_attempts(0),
m_successes(0),
m_transient(0),
m_errors(0),
m_maxGap(0),
m_lastSuccess(0),
m_finish(0)
{
    std::fill(m_statuses, m_statuses + LOCK_FREE_MAP_VERSION_EXHAUSTED + 1, UINT64_C(0));
}

void LockFreeMapProgressWorker::recordStatus(LockFreeMapStatus status)
{
    if (status >= LOCK_FREE_MAP_OK && status <= LOCK_FREE_MAP_VERSION_EXHAUSTED)
    {
        ++m_statuses[status];
    }
    else
    {
        ++m_errors;
    }
}

bool LockFreeMapProgressTest::attempt(LockFreeMap<int, uint64_t>& map, LockFreeMapProgressWorker& worker, unsigned mode, unsigned id)
{
    const bool pipeline = mode == 4 || mode == 5 || mode == 7;
    const bool producer = mode < 2 || (mode != 9 && id < 4);
    const int key = pipeline ? static_cast<int>((worker.m_attempts * 17 + id * 31) % 128) : 0;
    ++worker.m_attempts;
    LockFreeMapStatus status = LOCK_FREE_MAP_OK;
    uint64_t value = 0;
    LockFreeMapToken token;
    if (mode == 10)
    {
        status = map.lockFreeMapGet(0, value, token);
        if (status == LOCK_FREE_MAP_OK)
        {
            status = map.lockFreeMapUpdate(0, token, value + 1);
        }
        if (status == LOCK_FREE_MAP_COMMITTED)
        {
            return true;
        }
        if (status == LOCK_FREE_MAP_CONFLICT || status == LOCK_FREE_MAP_RECORD_LIMIT ||
            status == LOCK_FREE_MAP_CAPACITY || status == LOCK_FREE_MAP_RESERVED)
        {
            ++worker.m_transient;
        }
        else
        {
            ++worker.m_errors;
        }
        return false;
    }
    if (mode == 8)
    {
        // 每个线程反复删除并恢复自己的键，只竞争公共索引；同类线程都须取得进展
        if (!map.erase(static_cast<int>(id)))
        {
            ++worker.m_transient;
            return false;
        }
        if (!map.set(static_cast<int>(id), static_cast<uint64_t>(id)))
        {
            ++worker.m_errors;
            return false;
        }
        return true;
    }
    if (pipeline)
    {
        if (worker.m_items.size() == worker.m_items.capacity())
        {
            ++worker.m_errors;
            return false;
        }
        if (producer)
        {
            const uint64_t candidate = (static_cast<uint64_t>(id + 1) << 48) | (worker.m_successes + 1);
            status = map.lockFreeMapGetOrInsert(key, candidate, value, token);
            worker.recordStatus(status);
            if (status == LOCK_FREE_MAP_COMMITTED)
            {
                if (value != candidate)
                {
                    ++worker.m_errors;
                }
                worker.m_items.push_back(candidate);
                return true;
            }
        }
        else
        {
            if (mode == 7)
            {
                if (map.erase(key))
                {
                    return true;
                }
                ++worker.m_transient;
                return false;
            }
            if (mode == 4)
            {
                status = map.lockFreeMapExtract(key, value);
            }
            else
            {
                int highest = -1;
                status = map.lockFreeMapLast(highest, value, token);
                if (status == LOCK_FREE_MAP_OK)
                {
                    status = map.lockFreeMapExtractLast(token, highest, value);
                }
            }
            worker.recordStatus(status);
            if (status == LOCK_FREE_MAP_COMMITTED)
            {
                if ((value >> 48) < 1 || (value >> 48) > 4 || (value & UINT64_C(0x0000ffffffffffff)) == 0)
                {
                    ++worker.m_errors;
                }
                worker.m_items.push_back(value);
                return true;
            }
        }
    }
    else if (producer)
    {
        if (mode == 1)
        {
            if (map.set(0, static_cast<uint64_t>(id)))
            {
                return true;
            }
            // 旧 bool 接口不能区分暂时资源不足，单独保留失败次数，绝不算作成功
            ++worker.m_transient;
            return false;
        }
        status = map.lockFreeMapSet(0, static_cast<uint64_t>(id));
        if (status == LOCK_FREE_MAP_COMMITTED)
        {
            return true;
        }
    }
    else
    {
        if (mode == 3)
        {
#ifdef LOCK_FREE_MAP_PROGRESS_COMMON
            // 借用查询没有线上等价接口，不能用复制查询伪造比较
            ++worker.m_errors;
            return false;
#else
            unsigned callbacks = 0;
            // 参数只在借用期间有效；输出到线程私有变量并核对每次调用至多回调一次
            status = map.lockFreeMapVisit(0, [&value, &callbacks](const uint64_t& borrowed)
            {
                value = borrowed;
                ++callbacks;
            });
            if (callbacks != (status == LOCK_FREE_MAP_OK ? 1U : 0U))
            {
                ++worker.m_errors;
            }
#endif
        }
        else
        {
            status = map.lockFreeMapGet(0, value, token);
        }
        if (status == LOCK_FREE_MAP_OK)
        {
            if (value > 3)
            {
                ++worker.m_errors;
            }
            return true;
        }
    }
    if (status == LOCK_FREE_MAP_CONFLICT || status == LOCK_FREE_MAP_RECORD_LIMIT ||
        status == LOCK_FREE_MAP_RESERVED || status == LOCK_FREE_MAP_CAPACITY ||
        (pipeline && (status == LOCK_FREE_MAP_EXISTS || status == LOCK_FREE_MAP_NOT_FOUND)))
    {
        ++worker.m_transient;
    }
    else
    {
        ++worker.m_errors;
    }
    return false;
}

int LockFreeMapProgressTest::scenario(std::ostream& report, unsigned mode, unsigned milliseconds)
{
    static const char* const kNames[] = { "same_key_set", "same_key_legacy_set", "mixed_get", "mixed_visit", "pipeline_128", "priority_128", "record_limit_2", "pipeline_erase", "erase_restore", "readers_only", "conditional_update" };
    const unsigned workers = 8;
#ifdef LOCK_FREE_MAP_PROGRESS_COMMON
    // 两版均用共有构造及静止期释放接口，配置过程在计时之前完成
    LockFreeMap<int, uint64_t> map;
    map.clearAndFreeMemory();
#else
    LockFreeMap<int, uint64_t> map(false);
#endif
    LockFreeMapConfig config;
    config.nodes.initialCapacity = 256;
    config.values.initialCapacity = 512;
    config.nodes.maximumCapacity = 4096;
    config.values.maximumCapacity = 4096;
    if (mode == 6)
    {
        config.operationRecordLimit = 2;
    }
    if (map.lockFreeMapConfigure(config) != LOCK_FREE_MAP_OK)
    {
        return 1;
    }
    const bool pipeline = mode == 4 || mode == 5 || mode == 7;
    if (!pipeline && !map.set(0, 0))
    {
        return 1;
    }
    if (mode == 8)
    {
        for (unsigned id = 1; id < workers; ++id)
        {
            if (!map.set(static_cast<int>(id), static_cast<uint64_t>(id)))
            {
                return 1;
            }
        }
    }
    LockFreeMapProgressWorker results[workers];
    if (pipeline)
    {
        for (unsigned id = 0; id < workers; ++id)
        {
            results[id].m_items.reserve(1000000);
        }
    }
    std::atomic<unsigned> ready(0);
    std::atomic<bool> start(false);
    uint64_t begin = 0, deadline = 0;
    std::vector<LockFreeMapTestThread> threads;
    threads.reserve(workers);
    LockFreeMapTestThreadGroup cleanup(threads, &start);
    for (unsigned id = 0; id < workers; ++id)
    {
        // 共同起跑；不以固定成功次数结束，防止快线程退出后掩盖慢线程长期失败
        threads.emplace_back([&map, &results, &ready, &start, &begin, &deadline, mode, id]()
        {
            ready.fetch_add(1);
            while (!start.load())
            {
                std::this_thread::yield();
            }
            if (deadline == 0)
            {
                return;
            }
            LockFreeMapProgressWorker& result = results[id];
            result.m_lastSuccess = begin;
            uint64_t now = LockFreeMapBenchmarkClock::nanoseconds();
            while (now < deadline && result.m_errors == 0)
            {
                const bool success = attempt(map, result, mode, id);
                now = LockFreeMapBenchmarkClock::nanoseconds();
                if (success)
                {
                    ++result.m_successes;
                    result.m_maxGap = (std::max)(result.m_maxGap, now - result.m_lastSuccess);
                    result.m_lastSuccess = now;
                }
            }
            result.m_finish = now;
            result.m_maxGap = (std::max)(result.m_maxGap, now - result.m_lastSuccess);
        });
    }
    const uint64_t startupLimit = LockFreeMapBenchmarkClock::nanoseconds() + UINT64_C(5000000000);
    while (ready.load() != workers)
    {
        if (LockFreeMapBenchmarkClock::nanoseconds() >= startupLimit)
        {
            return 1;
        }
        std::this_thread::yield();
    }
    begin = LockFreeMapBenchmarkClock::nanoseconds();
    deadline = begin + static_cast<uint64_t>(milliseconds) * 1000000;
    start.store(true);
    int errors = 0;
    for (unsigned id = 0; id < workers; ++id)
    {
        errors += threads[id].join() ? 0 : 1;
    }
    uint64_t finish = deadline;
    for (unsigned id = 0; id < workers; ++id)
    {
        finish = (std::max)(finish, results[id].m_finish);
    }
    std::vector<uint64_t> produced, consumed;
    for (unsigned id = 0; id < workers; ++id)
    {
        LockFreeMapProgressWorker& result = results[id];
        const bool producer = mode < 2 || (mode != 9 && id < 4);
        // 独立键删除重插、覆盖及稳定读一直有可执行工作，要求逐线程成功
        // 混合流水线可能在整个调度窗口中只遇到空键或已存在，不能把正常返回误判成内部饥饿
        errors += result.m_errors != 0 || result.m_attempts == 0 || (!pipeline && result.m_successes == 0) ? 1 : 0;
        const char* const role = mode == 10 ? "conditional_update" : mode == 8 ? "erase_restore" : producer ? "producer" : pipeline ? "consumer" : "reader";
        report << "progress," << kNames[mode] << ',' << id << ',' << role << ','
            << result.m_attempts << ',' << result.m_successes << ',' << result.m_transient << ',' << result.m_errors << ','
            << result.m_maxGap << ',' << (finish - begin) << '\n';
        if (result.m_successes == 0)
        {
            report << "progress_zero_success," << kNames[mode] << ',' << id << ',' << result.m_attempts << '\n';
        }
        if (pipeline && (producer || mode != 7))
        {
            uint64_t counted = 0;
            for (unsigned status = 0; status <= LOCK_FREE_MAP_VERSION_EXHAUSTED; ++status)
            {
                counted += result.m_statuses[status];
                if (result.m_statuses[status] != 0)
                {
                    report << "progress_status," << kNames[mode] << ',' << id << ',' << status << ',' << result.m_statuses[status] << '\n';
                }
            }
            errors += counted == result.m_attempts ? 0 : 1;
        }
        if (pipeline)
        {
            std::vector<uint64_t>& target = producer ? produced : consumed;
            target.insert(target.end(), result.m_items.begin(), result.m_items.end());
        }
    }
    if (pipeline)
    {
        uint64_t consumerSuccesses = 0;
        for (unsigned id = 4; id < workers; ++id)
        {
            consumerSuccesses += results[id].m_successes;
        }
        // 流水线整体必须完成真实接纳与消费，空转不能冒充吞吐
        errors += !produced.empty() && consumerSuccesses != 0 ? 0 : 1;
        const std::vector<std::pair<int, uint64_t>> remaining = map.toVector();
        for (size_t index = 0; index < remaining.size(); ++index)
        {
            consumed.push_back(remaining[index].second);
        }
        std::sort(produced.begin(), produced.end());
        std::sort(consumed.begin(), consumed.end());
        if (mode == 7)
        {
            uint64_t erased = 0;
            for (unsigned id = 4; id < workers; ++id)
            {
                erased += results[id].m_successes;
            }
            // erase 不输出条目编号，独立核对成功接纳数等于成功删除数加最终条目数
            errors += produced.size() == erased + remaining.size() ? 0 : 1;
            errors += std::adjacent_find(produced.begin(), produced.end()) == produced.end() ? 0 : 1;
            for (size_t index = 0; index < consumed.size(); ++index)
            {
                errors += std::binary_search(produced.begin(), produced.end(), consumed[index]) ? 0 : 1;
            }
        }
        else
        {
            errors += produced == consumed && std::adjacent_find(produced.begin(), produced.end()) == produced.end() ? 0 : 1;
        }
    }
    else if (mode == 10)
    {
        uint64_t expected = 0, value = 0;
        for (unsigned id = 0; id < workers; ++id)
        {
            expected += results[id].m_successes;
        }
        errors += map.size() == 1 && map.get(0, value) && value == expected ? 0 : 1;
    }
    else if (mode == 8)
    {
        errors += map.size() == workers ? 0 : 1;
        for (unsigned id = 0; id < workers; ++id)
        {
            uint64_t value = 99;
            errors += map.get(static_cast<int>(id), value) && value == id ? 0 : 1;
        }
    }
    else
    {
        uint64_t value = 99;
        errors += map.size() == 1 && map.get(0, value) && value < (mode < 2 ? 8U : 4U) ? 0 : 1;
    }
    map.clearAndFreeMemory();
    const LockFreeMapStats stats = map.lockFreeMapStats();
    errors += stats.nodes.retained == 0 && stats.values.retained == 0 && stats.nodes.capacity == 0 && stats.values.capacity == 0 ? 0 : 1;
    return errors;
}

int LockFreeMapProgressTest::run(std::ostream& report, unsigned milliseconds)
{
    if (milliseconds == 0 || milliseconds > 60000)
    {
        return 1;
    }
    int errors = 0;
    const size_t threadErrors = LockFreeMapTestThread::m_errors.load();
    report << "kind,scenario,thread,role,attempts,successes,transient,errors,max_gap_ns,elapsed_ns\n";
    try
    {
        for (unsigned mode = 0; mode < 11; ++mode)
        {
#ifdef LOCK_FREE_MAP_PROGRESS_COMMON
            if (mode == 3)
            {
                continue;
            }
#endif
            errors += scenario(report, mode, milliseconds);
        }
    }
    catch (...)
    {
        ++errors;
    }
    errors += LockFreeMapTestThread::m_errors.load() == threadErrors ? 0 : 1;
    report << "progress_errors," << errors << '\n';
    return report.good() ? errors : errors + 1;
}