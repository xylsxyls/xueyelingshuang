#include "LockFreeHashMapRetryTests.h"
#include <cstring>
#include <functional>
#include <iostream>
#include <vector>

bool LockFreeHashMapGrowthTestCallback::s_throwCopy = false;
int32_t LockFreeHashMapGrowthTestCallback::s_live = 0;
int32_t LockFreeHashMapGrowthTestCallback::s_copies = 0;

LockFreeHashMapGrowthTestCallback::LockFreeHashMapGrowthTestCallback()
{
    std::memset(m_data, 37, sizeof(m_data));
    ++s_live;
}

LockFreeHashMapGrowthTestCallback::LockFreeHashMapGrowthTestCallback(const LockFreeHashMapGrowthTestCallback& other)
{
    ++s_copies;
    if (s_throwCopy)
    {
        throw 7;
    }
    std::memcpy(m_data, other.m_data, sizeof(m_data));
    ++s_live;
}

LockFreeHashMapGrowthTestCallback::~LockFreeHashMapGrowthTestCallback()
{
    --s_live;
}

void LockFreeHashMapGrowthTestCallback::operator()(size_t count, uint64_t total) const
{
    LockFreeHashMapTests::check(count != 0 && total != 0 && m_data[0] == 37 && m_data[511] == 37,
        "growth callback owns a complete target");
}

void LockFreeHashMapRetryTests::capacity()
{
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 1;
    config.maximumCapacity = 1;
    LockFreeHashMap<int32_t, int32_t> map(8, config);
    LockFreeHashMapTests::check(map.status() == LockFreeHashMapOk && map.set(1, 10), "one slot fixture");
    for (size_t mode = 0; mode < 2; ++mode)
    {
        LockFreeHashMapResult result = map.trySet(1, 20, nullptr, mode != 0, false);
        LockFreeHashMapTests::check(result.m_status == LockFreeHashMapCapacityLimit && !result.m_committed &&
            map.value(1) == 10, "nonwaiting update reports hard capacity and preserves old value");
    }
    LockFreeHashMap<int32_t, int32_t>::Snapshot snapshot;
    LockFreeHashMapTests::check(map.find(1, snapshot).m_status == LockFreeHashMapOk, "old version observed");
    LockFreeHashMapTests::check(!map.set(1, 20) && !map.insert(2, 20) &&
        map.compareExchange(1, snapshot.m_version, 20).m_status == LockFreeHashMapCapacityLimit &&
        map.insertIfAbsent(2, 20, snapshot).m_status == LockFreeHashMapCapacityLimit && map.value(1) == 10,
        "ordinary writes return resource shortage without waiting for themselves");
    bool rejected = false;
    try
    {
        map[1] = 20;
    }
    catch (const LockFreeHashMapAssignmentError& error)
    {
        rejected = error.status() == LockFreeHashMapCapacityLimit;
    }
    LockFreeHashMapTests::check(rejected && map.value(1) == 10, "subscript preserves explicit capacity failure");
    config.initialCapacity = 0;
    // 固定拒绝策略不依赖会改变的外部条件，普通接口必须正常结束。
    config.growPolicy = [](const LockFreeHashMapGrowContext&) -> size_t
    {
        return 0;
    };
    LockFreeHashMap<int32_t, int32_t> declined(8, config);
    LockFreeHashMapTests::check(!declined.set(1, 20) &&
        declined.trySet(1, 20).m_status == LockFreeHashMapGrowthDeclined, "permanent refusal is not hidden waiting");
    for (size_t mode = 0; mode < 4; ++mode)
    {
        size_t attempts = 0;
        // 单线程也能验证重试开关：第八次策略评估才允许申请真实容量。
        config.growPolicy = [&attempts](const LockFreeHashMapGrowContext&) -> size_t
        {
            return ++attempts >= 8 ? 1 : 0;
        };
        LockFreeHashMap<int32_t, int32_t> retry(8, config);
        LockFreeHashMapResult result = retry.trySet(1, 20, nullptr, (mode & 1) != 0, mode >= 2);
        LockFreeHashMapTests::check(mode >= 2 ? (result.m_committed && retry.value(1) == 20 && attempts == 8) :
            (!result.m_committed && result.m_status == LockFreeHashMapGrowthDeclined && attempts < 8 && retry.empty()),
            "capacity flag independently controls actual repeated allocation attempts");
    }
    uint32_t retries = 0;
    for (uint32_t i = 0; i < 4096; ++i)
    {
        LockFreeHashMapTests::check(LockFreeHashMapContention::waitCapacity(retries) && retries == i + 1,
            "capacity fast retry boundary");
    }
    for (size_t i = 0; i < 8; ++i)
    {
        double begin = LockFreeHashMapTests::timestamp();
        bool waited = LockFreeHashMapContention::waitCapacity(retries);
        double elapsed = LockFreeHashMapTests::timestamp() - begin;
        // 先保存独立时钟观测再输出，日志开销不能补足被测退避时间。
        std::cout << "CAPACITY_BACKOFF sample=" << i << " elapsed_us=" << elapsed * 1000000 << std::endl;
        LockFreeHashMapTests::check(waited && retries == 4096 && elapsed >= 0.001,
            "each saturated backoff waits at least one millisecond");
    }
}

void LockFreeHashMapRetryTests::waiting(bool retryContention)
{
    std::atomic<bool> entered(false), resume(false), stop(false), finished(false);
    std::atomic<size_t> attempts(0);
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 1;
    config.maximumCapacity = 1;
    // 第八次评估证明已超出单次 modify 的维护尝试，暂停点位于真实增长策略内。
    config.growPolicy = [&](const LockFreeHashMapGrowContext&) -> size_t
    {
        if (stop.load())
        {
            throw 7;
        }
        if (attempts.fetch_add(1) == 7)
        {
            entered.store(true);
            if (!LockFreeHashMapTests::wait(resume))
            {
                throw 7;
            }
        }
        return 1;
    };
    LockFreeHashMap<int32_t, int32_t> map(8, config);
    LockFreeHashMapTests::check(map.set(1, 10), "waiting fixture filled");
    LockFreeHashMapResult result;
    // 夹具保持到 join，异常由线程所有者计入设施失败。
    LockFreeHashMapTestThread producer([&]()
    {
        result = map.trySet(2, 20, nullptr, retryContention, true);
        finished.store(true);
    });
    try
    {
        LockFreeHashMapTests::check(LockFreeHashMapTests::wait(entered) && !finished.load(),
            "writer has reached the real repeated capacity path");
        LockFreeHashMapTests::check(map.erase(1), "consumer frees the old logical entry");
        resume.store(true);
        LockFreeHashMapTests::check(LockFreeHashMapTests::wait(finished), "waiting writer completes after capacity recovery");
        producer.join();
        LockFreeHashMapTests::check(result.m_committed && map.value(2) == 20 && map.size() == 1,
            "capacity recovery commits exactly once");
    }
    catch (...)
    {
        stop.store(true);
        resume.store(true);
        producer.join();
        throw;
    }
}

void LockFreeHashMapRetryTests::versions()
{
    LockFreeHashMap<int32_t, int32_t> map;
    LockFreeHashMap<int32_t, int32_t>::Snapshot snapshot;
    LockFreeHashMapTests::check(map.tryInsertIfAbsent(1, 1, snapshot, true, true).m_committed &&
        map.tryCompareExchange(1, snapshot.m_version, 2, nullptr, true, true).m_committed &&
        map.tryCompareExchange(1, snapshot.m_version, 3, nullptr, true, true).m_status == LockFreeHashMapVersionMismatch &&
        map.tryEraseIf(1, snapshot.m_version, true, true).m_status == LockFreeHashMapVersionMismatch &&
        map.tryTake(1, snapshot, true, true).m_committed && snapshot.m_value && *snapshot.m_value == 2 &&
        map.tryErase(1, true, true).m_status == LockFreeHashMapNotFound &&
        map.tryInsert(2, 4, true, true).m_committed && map.tryClear(true, true).m_committed && map.empty(),
        "retry flags do not refresh a stale version or repeat committed work");
    LockFreeHashMapPoolConfig config;
    config.protectionRecordCount = 1;
    LockFreeHashMap<int32_t, int32_t> nested(8, config);
    bool rejected = false;
    // 不允许等待自己退出回调才释放的唯一保护记录。
    LockFreeHashMapTests::check(nested.set(1, 7) && nested.forEachResult([&](const int32_t&, const int32_t&)
    {
        rejected = nested.trySet(2, 8, nullptr, true, true).m_status == LockFreeHashMapProtectionUnavailable;
    }).m_status == LockFreeHashMapOk && rejected && !nested.contains(2), "nested capacity wait refuses self dependency");
}

void LockFreeHashMapRetryTests::observers()
{
    long long baseline = LockFreeHashMapTests::s_liveAllocations.load();
    {
        LockFreeHashMapGrowthTestCallback target;
        LockFreeHashMapPoolConfig config;
        config.m_onGrowth = target;
        const int32_t copies = LockFreeHashMapGrowthTestCallback::s_copies;
        LockFreeHashMapGrowthTestCallback::s_throwCopy = true;
        try
        {
            LockFreeHashMap<int32_t, int32_t> map(8, config);
            LockFreeHashMapPoolConfig copied(config);
            LockFreeHashMapTests::check(map.status() == LockFreeHashMapOk &&
                map.configurePool(copied).m_status == LockFreeHashMapOk &&
                LockFreeHashMapGrowthTestCallback::s_copies == copies, "configuration only shares callback ownership");
            long long before = LockFreeHashMapTests::s_liveAllocations.load();
            bool failed = false;
            try
            {
                config.m_onGrowth = target;
            }
            catch (int)
            {
                failed = true;
            }
            LockFreeHashMapTests::check(failed && config.m_onGrowth &&
                LockFreeHashMapTests::s_liveAllocations.load() == before,
                "throwing callback preparation frees its allocation and preserves previous target");
        }
        catch (...)
        {
            LockFreeHashMapGrowthTestCallback::s_throwCopy = false;
            throw;
        }
        LockFreeHashMapGrowthTestCallback::s_throwCopy = false;
        const long long before = LockFreeHashMapTests::s_liveAllocations.load();
        bool failed = false;
        LockFreeHashMapTests::s_failAllocation.store(0);
        try
        {
            config.m_onGrowth = target;
        }
        catch (const std::bad_alloc&)
        {
            failed = true;
        }
        catch (...)
        {
            LockFreeHashMapTests::s_failAllocation.store(-1);
            throw;
        }
        LockFreeHashMapTests::s_failAllocation.store(-1);
        LockFreeHashMapTests::check(failed && config.m_onGrowth && LockFreeHashMapTests::s_liveAllocations.load() == before,
            "observer allocation failure preserves ownership without leaks");
        std::shared_ptr<std::function<void(size_t, uint64_t)>> shared =
            std::make_shared<std::function<void(size_t, uint64_t)>>(target);
        const int32_t sharedCopies = LockFreeHashMapGrowthTestCallback::s_copies;
        config.m_onGrowth = LockFreeHashMapGrowthObserver::fromShared(shared);
        config.m_onGrowth(1, 1);
        LockFreeHashMapTests::check(LockFreeHashMapGrowthTestCallback::s_copies == sharedCopies,
            "shared std function path never copies its erased target");
        config.m_onGrowth = nullptr;
        void (*empty)(size_t, uint64_t) = nullptr;
        config.m_onGrowth = empty;
        LockFreeHashMapTests::check(!config.m_onGrowth, "null callback pointers stay empty");
    }
    LockFreeHashMapTests::check(LockFreeHashMapGrowthTestCallback::s_live == 0 &&
        LockFreeHashMapTests::s_liveAllocations.load() == baseline, "all observer targets and allocations released");
    size_t calls = 0, slots = 0;
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 1;
    // 一次增长一个槽，精确核对非初始化通知和异常通知后的提交事实。
    config.growPolicy = [](const LockFreeHashMapGrowContext&) -> size_t
    {
        return 1;
    };
    config.m_onGrowth = [&](size_t count, uint64_t total)
    {
        ++calls;
        slots += count;
        LockFreeHashMapTests::check(count == 1 && total == calls, "sequential growth notification identity");
        throw 7;
    };
    LockFreeHashMap<int32_t, int32_t> map(8, config);
    LockFreeHashMapTests::check(calls == 0 && map.set(0, 0), "initial capacity is not a growth notification");
    for (int32_t i = 1; i <= 8; ++i)
    {
        LockFreeHashMapTests::check(map.set(i, i), "throwing notification does not revoke a published allocation");
    }
    LockFreeHashMapPoolStats stats = {};
    LockFreeHashMapTests::check(map.poolStats(stats).m_status == LockFreeHashMapOk && calls == 8 && slots == 8 &&
        stats.successfulGrowCount == calls && stats.capacity == 9, "growth notification exactly matches published slots");
}

void LockFreeHashMapRetryTests::assignments()
{
    LockFreeHashMap<int32_t, int32_t> map(1);
    LockFreeHashMapTests::check(map.set(1, 0), "assignment contention fixture");
    std::atomic<bool> start(false);
    std::atomic<size_t> completed(0);
    std::vector<LockFreeHashMapTestThread> workers;
    workers.reserve(8);
    try
    {
        for (size_t i = 0; i < 8; ++i)
        {
            // 相同下标 API；异常由线程所有者记录，不能当作成功。
            workers.push_back(LockFreeHashMapTestThread([&]()
            {
                if (!LockFreeHashMapTests::wait(start))
                {
                    throw 7;
                }
                for (int32_t j = 0; j < 20000; ++j)
                {
                    map[1] = j;
                    completed.fetch_add(1);
                }
            }));
        }
    }
    catch (...)
    {
        start.store(true);
        throw;
    }
    start.store(true);
    for (size_t i = 0; i < workers.size(); ++i)
    {
        workers[i].join();
    }
    LockFreeHashMapTests::check(completed.load() == 160000 && map.value(1) == 19999,
        "all concurrent subscript assignments commit without contention exceptions");
}

void LockFreeHashMapRetryTests::concurrentGrowth()
{
    std::atomic<size_t> calls(0), slots(0);
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 0;
    // 固定小批次制造并发发布通知，不把首次预分配当成通知。
    config.growPolicy = [](const LockFreeHashMapGrowContext&) -> size_t
    {
        return 1;
    };
    config.m_onGrowth = [&](size_t count, uint64_t total)
    {
        LockFreeHashMapTests::check(count != 0 && total != 0, "concurrent growth publishes a real batch");
        calls.fetch_add(1);
        slots.fetch_add(count);
    };
    LockFreeHashMap<int32_t, int32_t> map(8, config);
    std::atomic<bool> start(false);
    std::vector<LockFreeHashMapTestThread> workers;
    workers.reserve(8);
    try
    {
        for (int32_t i = 0; i < 8; ++i)
        {
            // 工作线程独占键区间，共享池；join 后比较通知和容量总量。
            workers.push_back(LockFreeHashMapTestThread([&, i]()
            {
                if (!LockFreeHashMapTests::wait(start))
                {
                    throw 7;
                }
                for (int32_t j = 0; j < 8; ++j)
                {
                    LockFreeHashMapTests::check(map.set(i * 8 + j, i * 8 + j), "concurrent insertion commits");
                }
            }));
        }
    }
    catch (...)
    {
        start.store(true);
        throw;
    }
    start.store(true);
    for (size_t i = 0; i < workers.size(); ++i)
    {
        workers[i].join();
    }
    LockFreeHashMapPoolStats stats = {};
    LockFreeHashMapTests::check(map.poolStats(stats).m_status == LockFreeHashMapOk && map.size() == 64 &&
        calls.load() == stats.successfulGrowCount && slots.load() == stats.capacity,
        "concurrent notifications account for every published growth batch");
}

bool LockFreeHashMapRetryTests::run()
{
    capacity();
    versions();
    observers();
#ifndef LOCKFREEHASHMAP_TEST_SINGLE_THREAD
    waiting(false);
    waiting(true);
    assignments();
    concurrentGrowth();
#else
    std::cout << "NOT_RUN subcase=retry_threads reason=single_thread_host" << std::endl;
#endif
    return true;
}

bool LockFreeHashMapRetryTest()
{
    return LockFreeHashMapRetryTests::run();
}