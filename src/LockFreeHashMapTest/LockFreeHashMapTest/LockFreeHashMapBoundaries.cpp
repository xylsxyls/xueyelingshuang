#include "LockFreeHashMapTests.h"
#include "LockFreeHashMapTestState.h"
#include <stdexcept>

bool LockFreeHashMapTestKey::s_fail = false;

LockFreeHashMapTestKey::LockFreeHashMapTestKey(int value) :
m_value(value)
{

}

LockFreeHashMapTestKey::LockFreeHashMapTestKey(const LockFreeHashMapTestKey& other) :
m_value(other.m_value)
{
    if (s_fail)
    {
        throw std::runtime_error("key copy");
    }
}

size_t LockFreeHashMapKeyHash::operator()(const LockFreeHashMapTestKey& key) const
{
    return static_cast<size_t>(key.m_value);
}

bool LockFreeHashMapKeyEqual::operator()(const LockFreeHashMapTestKey& left, const LockFreeHashMapTestKey& right) const
{
    return left.m_value == right.m_value;
}

bool LockFreeHashMapTestAccess::exhaustion()
{
    LockFreeHashMap<int, int> map;
    LockFreeHashMapTestState::sequence(map).store((std::numeric_limits<uint64_t>::max)() - 1);
    LockFreeHashMap<int, int>::Snapshot snapshot;
    if (!map.insertIfAbsent(1, 10, snapshot).m_committed ||
        snapshot.m_version.m_sequence != (std::numeric_limits<uint64_t>::max)() ||
        map.trySet(1, 20).m_status != LockFreeHashMapVersionExhausted ||
        map.value(1) != 10 || !map.eraseIf(1, snapshot.m_version).m_committed)
    {
        return false;
    }
    LockFreeHashMapTestState::generation(map).store((std::numeric_limits<uint64_t>::max)());
    LockFreeHashMapResult cleared = map.tryClear();
    bool generationSafe = cleared.m_committed && cleared.m_status == LockFreeHashMapVersionExhausted &&
        map.empty() && map.trySet(1, 2).m_status == LockFreeHashMapVersionExhausted &&
        !map.tryClear().m_committed && LockFreeHashMapTestState::generation(map).load() == 0;
    return generationSafe;
}

void LockFreeHashMapTests::boundaries()
{
    LockFreeHashMapTestAccess::optimizations();
    check(LockFreeHashMapTestAccess::exhaustion(), "map identity exhaustion never wraps");
    LockFreeHashMap<int, int> cold(1024);
    LockFreeHashMapPoolConfig config;
    config.inlineScalarValues = false;
    config.initialCapacity = 2;
    config.maximumCapacity = 2;
    config.directoryCapacity = 2;
    check(cold.configurePool(config).m_status == LockFreeHashMapOk, "cold clear fixture");
    for (int i = 0; i < 200; ++i)
    {
        check(cold.set((i * 73) % 1024, i) && cold.tryClear().m_committed, "cold clear commits at fixed capacity");
        cold.maintain(1024);
        cold.maintain(1024);
        check(cold.retainedNodeCount() == 0, "maintenance recovers cold old-generation buckets at fixed capacity");
    }
    typedef LockFreeHashMap<LockFreeHashMapTestKey, int, LockFreeHashMapKeyHash, LockFreeHashMapKeyEqual> KeyMap;
    KeyMap keys;
    LockFreeHashMapTestKey first(1);
    LockFreeHashMapTestKey second(2);
    check(keys.set(first, 10), "key fixture");
    LockFreeHashMapTestKey::s_fail = true;
    check(keys.trySet(second, 20).m_status == LockFreeHashMapUserException && !keys.contains(second), "key copy fails before publication");
    check(keys.configurePool(config).m_status == LockFreeHashMapNotEmpty && keys.value(first) == 10,
        "configuration cannot erase a live map when key copy throws");
    LockFreeHashMapTestKey::s_fail = false;
    check(keys.set(second, 20), "key copy failure retry");

    LockFreeHashMap<int, int> single;
    config.initialCapacity = 4;
    config.maximumCapacity = 4;
    config.protectionRecordCount = 1;
    single.configurePool(config);
    single.set(1, 10);
    LockFreeHashMapResult nested;
    // key、value 只在回调内借用；回调执行本场景校验或异常注入，无返回值
    LockFreeHashMapResult outer = single.forEachResult([&](const int& key, const int& value)
    {
        (void)key;
        (void)value;
        LockFreeHashMap<int, int>::Snapshot output;
        nested = single.find(1, output);
        single.clear();
    });
    check(outer.m_status == LockFreeHashMapOk && nested.m_status == LockFreeHashMapProtectionUnavailable &&
        single.empty() && single.set(2, 20), "guard exhaustion returns without waiting and clear still commits");

    LockFreeHashMap<int, int> allocating;
    config.initialCapacity = 0;
    config.maximumCapacity = 2;
    config.protectionRecordCount = 4;
    // context 为本次池状态快照；返回该场景要求的增长数量或收缩决策
    config.growPolicy = [](const LockFreeHashMapGrowContext& context)
    {
        (void)context;
        return size_t(2);
    };
    allocating.configurePool(config);
    std::atomic<bool> start(false);
    LockFreeHashMapResult creator;
    s_allocationEntered.store(false);
    s_allocationResume.store(false);
    // 无参数工作函数借用本用例夹具，结果在连接线程后读取，无返回值
    LockFreeHashMapTestThread pending([&]()
    {
        wait(start);
        creator = allocating.trySet(1, 10);
    });
    LockFreeHashMapTestResume resumeOnExit(s_allocationResume, pending);
    s_pauseAllocation.store(0);
    start.store(true);
    bool entered = wait(s_allocationEntered);
    check(entered, "allocator paused after hard budget reservation");
    LockFreeHashMapPoolStats metrics = {};
    allocating.poolStats(metrics);
    LockFreeHashMapResult competitor = allocating.trySet(2, 20);
    check(metrics.budget == 2 && metrics.peakBudget == 2 && metrics.reservedCapacity == 2 &&
        metrics.capacity == 0 && competitor.m_status == LockFreeHashMapCapacityLimit && !competitor.m_committed,
        "paused allocation owns bounded reservation; competitor reports capacity limit promptly");
    s_allocationResume.store(true);
    pending.join();
    check(creator.m_committed && allocating.value(1) == 10 && !allocating.contains(2), "reservation owner publishes after resume");
    allocating.poolStats(metrics);
    check(metrics.peakBudget == 2 && metrics.budget == 2 && metrics.reservedCapacity == 0, "reservation returned on publication");

    LockFreeHashMap<int, int> growing;
    config.initialCapacity = 0;
    config.maximumCapacity = 128;
    config.directoryCapacity = 64;
    config.protectionRecordCount = 16;
    config.growFreeThreshold = 1;
    // context 为本次池状态快照；返回该场景要求的增长数量或收缩决策
    config.growPolicy = [](const LockFreeHashMapGrowContext& context)
    {
        return (std::max)(size_t(2), context.currentCapacity / 3);
    };
    growing.configurePool(config);
    std::atomic<int> growthFailures(0);
    std::vector<LockFreeHashMapTestThread> writers;
    for (int worker = 0; worker < 8; ++worker)
    {
        // 无参数工作函数借用本用例夹具，结果在连接线程后读取，无返回值
        writers.push_back(LockFreeHashMapTestThread([&, worker]()
        {
            for (int i = 0; i < 8; ++i)
            {
                bool committed = false;
                for (int retry = 0; retry < 100000 && !committed; ++retry)
                {
                    LockFreeHashMapResult result = growing.trySet(worker * 8 + i, i);
                    committed = result.m_committed;
                    if (!committed && result.m_status != LockFreeHashMapCapacityLimit)
                    {
                        break;
                    }
                }
                if (!committed)
                {
                    growthFailures.fetch_add(1);
                }
            }
        }));
    }
    for (size_t i = 0; i < writers.size(); ++i)
    {
        writers[i].join();
    }
    growing.poolStats(metrics);
    check(growthFailures.load() == 0 && growing.size() == 64 && metrics.peakBudget <= 128 && metrics.reservedCapacity == 0,
        "concurrent proportional growth obeys combined budget and preserves all keys");
}