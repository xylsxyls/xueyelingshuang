#include "LockFreeHashMapTests.h"
#include "LockFreeHashMapTestState.h"
#include <cstring>
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>

std::atomic<bool> LockFreeHashMapOnceEqual::s_throw(false);
std::atomic<int> LockFreeHashMapOnceEqual::s_calls(0);

bool LockFreeHashMapOnceEqual::operator()(int left, int right) const
{
    s_calls.fetch_add(1);
    if (s_throw.exchange(false))
    {
        throw std::runtime_error("one equality failure");
    }
    return left == right;
}

void LockFreeHashMapTestAccess::sequence(LockFreeHashMap<int, int>& map, uint64_t value)
{
    LockFreeHashMapTestState::sequence(map).store(value);
    LockFreeHashMap<int, int>::Storage& storage = LockFreeHashMapTestState::storage(map);
    for (size_t i = 0; i < storage.epoch()->count(); ++i)
    {
        LockFreeHashMapEpochRecord* record = storage.epoch()->record(i);
        record->m_versionNext = 0;
        record->m_versionEnd = 0;
    }
}

void LockFreeHashMapTestAccess::historySequence(LockFreeHashMap<int, int, LockFreeHashMapCollisionHash>& map, uint64_t value)
{
    LockFreeHashMapTestState::historySequence(map).store(value);
}

void LockFreeHashMapTestAccess::countOptimizations()
{
    LockFreeHashMap<int, int> map;
    LockFreeHashMapTests::check(map.set(1, 10), "sharded count fixture");
    LockFreeHashMapEpochRecord& record = *LockFreeHashMapTestState::storage(map).epoch()->record(0);
    record.m_countGeneration.store(LockFreeHashMapTestState::generation(map).load());
    record.m_countDelta.store((std::numeric_limits<int64_t>::max)());
    LockFreeHashMapTestState::account(map, record, LockFreeHashMapTestState::generation(map).load(), 1);
    LockFreeHashMapTests::check(!LockFreeHashMapTestState::countValid(map).load() && map.count() == 1,
        "sharded count overflow falls back to protected traversal");
    map.clear();
    LockFreeHashMapTestState::account(map, record, 1, -1);
    LockFreeHashMapTests::check(map.count() == 0 && map.set(2, 20) && map.count() == 1,
        "late old-generation accounting cannot restore cleared keys");
    map.clearAndFreeMemory();
    LockFreeHashMapPoolConfig config;
    LockFreeHashMapTests::check(map.configurePool(config).m_status == LockFreeHashMapOk &&
        LockFreeHashMapTestState::countValid(map).load() && map.count() == 0,
        "quiescent pool reconfiguration restores compact accounting");
}

void LockFreeHashMapTestAccess::optimizations()
{
    countOptimizations();
    readOptimizations();
    LockFreeHashMap<bool, bool> flags;
    LockFreeHashMap<unsigned char, unsigned short> subword;
    bool flag = false;
    unsigned short shortValue = 0;
    LockFreeHashMapTests::check(flags.set(true, false) && flags.get(true, flag) && !flag &&
        subword.set(0xa5, 0xc3d2) && subword.get(0xa5, shortValue) && shortValue == 0xc3d2,
        "boolean and subword copies preserve complete values");
    LockFreeHashMap<int, int, LockFreeHashMapCollisionHash, LockFreeHashMapModuloEqual> equivalent(1);
    LockFreeHashMapTests::check(equivalent.set(1, 10) && equivalent.set(11, 20) &&
        equivalent.size() == 1 && equivalent.value(21) == 20,
        "custom equivalent keys remain one entry without scalar cache bypass");
    LockFreeHashMap<int, int, LockFreeHashMapCollisionHash, LockFreeHashMapOnceEqual> throwing(1);
    throwing.set(1, 10);
    throwing.set(2, 20);
    LockFreeHashMapOnceEqual::s_calls.store(0);
    LockFreeHashMapOnceEqual::s_throw.store(true);
    LockFreeHashMapResult failed = throwing.trySet(1, 99);
    bool stopped = LockFreeHashMapOnceEqual::s_calls.load() == 1;
    LockFreeHashMapTests::check(stopped && failed.m_status == LockFreeHashMapUserException && !failed.m_committed &&
        throwing.value(1) == 10, "one-shot search exception is propagated without hidden retry");
#ifndef LOCKFREEHASHMAP_TEST_SINGLE_THREAD
    sharedContention();
#endif
}

std::atomic<int> LockFreeHashMapReadPayload::s_live(0);
std::atomic<int> LockFreeHashMapReadPayload::s_copies(0);
std::atomic<bool> LockFreeHashMapReadPayload::s_throwCopy(false);
std::atomic<int> LockFreeHashMapReadPayload::s_assignments(0);
std::atomic<bool> LockFreeHashMapReadPayload::s_pause(false);
std::atomic<bool> LockFreeHashMapReadPayload::s_throw(false);
std::atomic<bool> LockFreeHashMapReadPayload::s_entered(false);
std::atomic<bool> LockFreeHashMapReadPayload::s_resume(false);

LockFreeHashMapReadPayload::LockFreeHashMapReadPayload(int value) :
m_value(value)
{
    s_live.fetch_add(1);
}

LockFreeHashMapReadPayload::LockFreeHashMapReadPayload(const LockFreeHashMapReadPayload& other) :
m_value(other.m_value)
{
    s_copies.fetch_add(1);
    if (s_throwCopy.load())
    {
        throw std::runtime_error("injected read payload copy");
    }
    s_live.fetch_add(1);
}

LockFreeHashMapReadPayload& LockFreeHashMapReadPayload::operator=(const LockFreeHashMapReadPayload& other)
{
    s_assignments.fetch_add(1);
    if (s_throw.exchange(false))
    {
        throw std::runtime_error("one output assignment failure");
    }
    if (s_pause.exchange(false))
    {
        s_entered.store(true);
        if (!LockFreeHashMapTests::wait(s_resume, 10000))
        {
            throw std::runtime_error("paused output assignment timeout");
        }
    }
    m_value = other.m_value;
    return *this;
}

LockFreeHashMapReadPayload::~LockFreeHashMapReadPayload()
{
    s_live.fetch_sub(1);
}

LockFreeHashMapMoveCallback::LockFreeHashMapMoveCallback(int& calls, bool throwOnMove) :
m_calls(&calls), m_throwOnMove(throwOnMove)
{

}

LockFreeHashMapMoveCallback::LockFreeHashMapMoveCallback(LockFreeHashMapMoveCallback&& other) :
m_calls(other.m_calls), m_throwOnMove(other.m_throwOnMove)
{
    if (m_throwOnMove)
    {
        throw std::runtime_error("injected callback move");
    }
    other.m_calls = nullptr;
}

void LockFreeHashMapMoveCallback::operator()(const int& key, const LockFreeHashMapReadPayload& value) const
{
    (void)key;
    (void)value;
    if (m_calls != nullptr)
    {
        ++*m_calls;
    }
}

bool LockFreeHashMapModuloEqual::operator()(int left, int right) const
{
    return left % 10 == right % 10;
}

void LockFreeHashMapTestAccess::ignoreReadPayload(const int& key, const LockFreeHashMapReadPayload& value)
{
    (void)key;
    (void)value;
}

void LockFreeHashMapTestAccess::readOptimizations()
{
    typedef LockFreeHashMap<int, LockFreeHashMapReadPayload> Map;
    {
        Map map(1);
        LockFreeHashMapPoolConfig config;
        config.lookupCacheCapacity = 0;
        LockFreeHashMapTests::check(map.configurePool(config).m_status == LockFreeHashMapOk &&
            map.set(1, LockFreeHashMapReadPayload(42)), "uncached read fixture ready");
        LockFreeHashMapReadPayload output;
        int copies = LockFreeHashMapReadPayload::s_copies.load();
        int assignments = LockFreeHashMapReadPayload::s_assignments.load();
        LockFreeHashMapReadPayload::s_throwCopy.store(true);
        bool read = map.get(1, output);
        LockFreeHashMapReadPayload::s_throwCopy.store(false);
        LockFreeHashMapTests::check(read && output.m_value == 42 &&
            LockFreeHashMapReadPayload::s_copies.load() == copies &&
            LockFreeHashMapReadPayload::s_assignments.load() == assignments + 1,
            "uncached get assigns once without constructing a temporary value");
        LockFreeHashMapReadPayload::s_throw.store(true);
        LockFreeHashMapTests::check(!map.get(1, output) &&
            LockFreeHashMapReadPayload::s_assignments.load() == assignments + 2 && map.contains(1),
            "uncached get reports assignment failure once and preserves the entry");
        // key、value 为调用期借用，捕获对象只用于注入回调复制失败，不执行输出修改
        auto callback = [output](const int& key, const LockFreeHashMapReadPayload& value)
        {
            (void)key;
            (void)value;
            (void)output;
        };
        LockFreeHashMapReadPayload::s_throwCopy.store(true);
        LockFreeHashMapResult walked = map.forEachResult(callback);
        size_t completed = map.forEach(callback);
        LockFreeHashMapReadPayload::s_throwCopy.store(false);
        LockFreeHashMapTests::check(walked.m_status == LockFreeHashMapUserException &&
            walked.m_count == 0 && completed == 0 && map.contains(1),
            "traversal catches callback copy failure before any invocation");
        // 借用已有 std::function，避免旧标准库复制目标失败时丢失内部申请的内存。
        // 开启目标复制故障后仍应成功，证明引用包装没有偷偷复制目标。
        std::function<void(const int&, const LockFreeHashMapReadPayload&)> ownedCallback(callback);
        copies = LockFreeHashMapReadPayload::s_copies.load();
        LockFreeHashMapReadPayload::s_throwCopy.store(true);
        walked = map.forEachResult(std::ref(ownedCallback));
        completed = map.forEach(std::cref(ownedCallback));
        LockFreeHashMapReadPayload::s_throwCopy.store(false);
        LockFreeHashMapTests::check(walked.m_status == LockFreeHashMapOk && walked.m_count == 1 &&
            completed == 1 && LockFreeHashMapReadPayload::s_copies.load() == copies,
            "reference-wrapped std::function traverses without copying its throwing target");
        int calls = 0;
        LockFreeHashMapMoveCallback movable(calls, false);
        walked = map.forEachResult(std::move(movable));
        LockFreeHashMapMoveCallback legacy(calls, false);
        completed = map.forEach(std::move(legacy));
        LockFreeHashMapTests::check(walked.m_status == LockFreeHashMapOk && walked.m_count == 1 &&
            completed == 1 && calls == 2, "move-only callback works through status and legacy traversal");
        LockFreeHashMapMoveCallback throwing(calls, true);
        walked = map.forEachResult(std::move(throwing));
        LockFreeHashMapMoveCallback throwingLegacy(calls, true);
        completed = map.forEach(std::move(throwingLegacy));
        LockFreeHashMapTests::check(walked.m_status == LockFreeHashMapUserException && walked.m_count == 0 &&
            completed == 0 && calls == 2 && map.contains(1), "callback move failure is caught without invoking or modifying the entry");
        walked = map.forEachResult(ignoreReadPayload);
        LockFreeHashMapTests::check(walked.m_status == LockFreeHashMapOk && walked.m_count == 1 &&
            map.forEach(ignoreReadPayload) == 1 && map.forEachResult<decltype(callback)>(callback).m_count == 1,
            "function references and explicitly selected lvalue callback templates retain their calling contract");
    }
}

void LockFreeHashMapTestAccess::sharedContention()
{
    typedef LockFreeHashMap<int, std::shared_ptr<int>, LockFreeHashMapCollisionHash> Map;
    Map map(1);
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 512;
    config.maximumCapacity = 512;
    config.protectionRecordCount = 16;
    config.lookupCacheCapacity = 4;
    LockFreeHashMapTests::check(map.configurePool(config).m_status == LockFreeHashMapOk,
        "shared same-key batch contention configuration");
    std::vector<std::shared_ptr<int>> values;
    std::vector<std::weak_ptr<int>> weak;
    values.reserve(64);
    weak.reserve(64);
    for (int i = 0; i < 64; ++i)
    {
        values.push_back(std::make_shared<int>(i));
        weak.push_back(values.back());
    }
    for (int i = 0; i < 4; ++i)
    {
        map.set(i, values[i]);
    }
    std::atomic<int> errors(0);
    uint64_t statuses[4][16] = {};
    std::atomic<bool> start(false);
    std::atomic<bool> finished(false);
    std::vector<LockFreeHashMapTestThread> threads;
    threads.reserve(4);
    LockFreeHashMapTestThread maintenance;
    try
    {
        // 无参数工作函数借用本用例夹具，结果在连接线程后读取，无返回值
        maintenance = LockFreeHashMapTestThread([&]()
        {
            try
            {
                while (!finished.load())
                {
                    if (map.maintain(17).m_status != LockFreeHashMapOk)
                    {
                        errors.fetch_add(1);
                    }
                    LockFreeHashMapPoolStats stats = {};
                    if (map.poolStats(stats).m_status != LockFreeHashMapOk || stats.budget > 512 || stats.peakBudget > 512)
                    {
                        errors.fetch_add(1);
                    }
                    // key、value 只在回调内借用；回调执行本场景校验或异常注入，无返回值
                    LockFreeHashMapResult walked = map.forEachResult([&](const int& key, const std::shared_ptr<int>& value)
                    {
                        if (key < 0 || key >= 4 || !value || *value < 0 || *value >= 64 || *value % 4 != key)
                        {
                            errors.fetch_add(1);
                        }
                    });
                    if (walked.m_status != LockFreeHashMapOk)
                    {
                        errors.fetch_add(1);
                    }
                    LockFreeHashMapTests::yieldThread();
                }
            }
            catch (...)
            {
                errors.fetch_add(1);
            }
        });
        for (int t = 0; t < 4; ++t)
        {
            // 无参数工作函数借用本用例夹具，结果在连接线程后读取，无返回值
            threads.push_back(LockFreeHashMapTestThread([&, t]()
            {
                try
                {
                    LockFreeHashMapTests::wait(start);
                    uint32_t random = UINT32_C(0x51a7c00d) + t;
                    for (int i = 0; i < 20000; ++i)
                    {
                        random = random * UINT32_C(1664525) + UINT32_C(1013904223);
                        int key = int((random >> 8) & 3);
                        const std::shared_ptr<int>& candidate = values[int((random >> 16) & 15) * 4 + key];
                        LockFreeHashMapResult result;
                        if (i % 257 == 0)
                        {
                            result = map.tryClear();
                            if (result.m_status != LockFreeHashMapOk || !result.m_committed)
                            {
                                errors.fetch_add(1);
                            }
                        }
                        else if (i % 13 == 0)
                        {
                            Map::Snapshot observed;
                            result = map.find(key, observed);
                            if (result.m_status == LockFreeHashMapOk)
                            {
                                if (!observed.m_value || !*observed.m_value || **observed.m_value % 4 != key)
                                {
                                    errors.fetch_add(1);
                                }
                                result = map.compareExchange(key, observed.m_version, candidate);
                                if (!result.m_committed && result.m_status != LockFreeHashMapNotFound &&
                                    result.m_status != LockFreeHashMapVersionMismatch && result.m_status != LockFreeHashMapCleared &&
                                    result.m_status != LockFreeHashMapCapacityLimit && result.m_status != LockFreeHashMapReservationBusy)
                                {
                                    errors.fetch_add(1);
                                }
                            }
                            else if (result.m_status != LockFreeHashMapNotFound && result.m_status != LockFreeHashMapCleared)
                            {
                                errors.fetch_add(1);
                            }
                        }
                        else if (i % 11 == 0)
                        {
                            Map::Snapshot actual;
                            result = map.insertIfAbsent(key, candidate, actual);
                            if (result.m_status == LockFreeHashMapOk || result.m_status == LockFreeHashMapExists)
                            {
                                if (!actual.m_value || !*actual.m_value || **actual.m_value % 4 != key ||
                                    result.m_committed != (result.m_status == LockFreeHashMapOk))
                                {
                                    errors.fetch_add(1);
                                }
                            }
                            else if ((result.m_status != LockFreeHashMapCleared && result.m_status != LockFreeHashMapCapacityLimit &&
                                result.m_status != LockFreeHashMapReservationBusy) || result.m_committed)
                            {
                                errors.fetch_add(1);
                            }
                        }
                        else if (i % 7 == 0)
                        {
                            result = map.tryErase(key);
                            if (result.m_status != LockFreeHashMapOk && result.m_status != LockFreeHashMapNotFound &&
                                result.m_status != LockFreeHashMapCleared)
                            {
                                errors.fetch_add(1);
                            }
                        }
                        else if (i % 3 == 0)
                        {
                            std::shared_ptr<int> got;
                            if (map.get(key, got) && (!got || *got < 0 || *got >= 64 || *got % 4 != key || got != values[*got]))
                            {
                                errors.fetch_add(1);
                            }
                        }
                        else
                        {
                            result = map.trySet(key, candidate);
                            if ((!result.m_committed || result.m_status != LockFreeHashMapOk) &&
                                (result.m_committed || (result.m_status != LockFreeHashMapCleared &&
                                result.m_status != LockFreeHashMapCapacityLimit && result.m_status != LockFreeHashMapReservationBusy)))
                            {
                                errors.fetch_add(1);
                            }
                        }
                        if (result.m_committed && result.m_status != LockFreeHashMapOk)
                        {
                            errors.fetch_add(1);
                        }
                        if (static_cast<unsigned>(result.m_status) < 16)
                        {
                            // 每个工作线程只写自己的统计行，避免诊断的共享原子争用掩盖交错
                            ++statuses[t][result.m_status];
                        }
                    }
                }
                catch (...)
                {
                    errors.fetch_add(1);
                }
            }));
        }
    }
    catch (...)
    {
        errors.fetch_add(1);
    }
    start.store(true);
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    finished.store(true);
    if (maintenance.joinable())
    {
        maintenance.join();
    }
    map.clear();
    for (size_t i = 0; i < 8; ++i)
    {
        map.maintain(64);
    }
    LockFreeHashMapPoolStats reclaimed = {};
    bool reclaimedAll = map.poolStats(reclaimed).m_status == LockFreeHashMapOk && reclaimed.occupied == 0 && reclaimed.retired == 0;
    map.clearAndFreeMemory();
    values.clear();
    bool expired = true;
    for (size_t i = 0; i < weak.size(); ++i)
    {
        expired = expired && weak[i].expired();
    }
    std::cout << "SHARED_STRESS workers=" << threads.size() << " operations=80000 errors=" << errors.load()
        << " all_expired=" << expired << std::endl;
    for (size_t i = 0; i < 16; ++i)
    {
        uint64_t count = 0;
        for (size_t t = 0; t < 4; ++t)
        {
            count += statuses[t][i];
        }
        std::cout << "SHARED_STATUS status=" << i << " count=" << count << std::endl;
    }
    LockFreeHashMapPoolStats finalStats = {};
    LockFreeHashMapTests::check(threads.size() == 4 && errors.load() == 0 && expired && reclaimedAll &&
        map.poolStats(finalStats).m_status == LockFreeHashMapOk && finalStats.capacity == 0,
        "80000 shared same-key operations with clear conditional writes and concurrent maintenance preserve ownership and budgets");
}