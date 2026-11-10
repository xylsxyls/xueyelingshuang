#include "LockFreeHashMapTests.h"
#include "LockFreeHashMapTestEntry.h"
#include "LockFreeHashMapMigration.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>
#if defined(_WIN32)
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>
#include <TlHelp32.h>
#pragma comment(lib, "Psapi.lib")
#endif

LockFreeHashMapTestResources::LockFreeHashMapTestResources() :
m_handles(0), m_threads(0), m_peakWorkingSet(0), m_cpuSeconds(0)
{
#if defined(_WIN32)
    GetProcessHandleCount(GetCurrentProcess(), &m_handles);
    PROCESS_MEMORY_COUNTERS memory = {};
    memory.cb = sizeof(memory);
    if (GetProcessMemoryInfo(GetCurrentProcess(), &memory, sizeof(memory)))
    {
        m_peakWorkingSet = memory.PeakWorkingSetSize;
    }
    FILETIME creation = {}, exit = {}, kernel = {}, user = {};
    if (GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user))
    {
        uint64_t ticks = (uint64_t(kernel.dwHighDateTime) << 32) + kernel.dwLowDateTime +
            (uint64_t(user.dwHighDateTime) << 32) + user.dwLowDateTime;
        m_cpuSeconds = double(ticks) / 10000000;
    }
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot != INVALID_HANDLE_VALUE)
    {
        THREADENTRY32 entry = {};
        entry.dwSize = sizeof(entry);
        if (Thread32First(snapshot, &entry))
        {
            do
            {
                m_threads += entry.th32OwnerProcessID == GetCurrentProcessId() ? 1 : 0;
            } while (Thread32Next(snapshot, &entry));
        }
        CloseHandle(snapshot);
    }
#endif
}

std::atomic<int> LockFreeHashMapTests::s_failures(0);
std::atomic<int> LockFreeHashMapTests::s_checks(0);
bool LockFreeHashMapTests::s_tracking = false;
LOCKFREEHASHMAP_TEST_ALIGN_64 std::atomic<long long> LockFreeHashMapTests::s_failAllocation(-1);
LOCKFREEHASHMAP_TEST_ALIGN_64 std::atomic<long long> LockFreeHashMapTests::s_allocations(0);
LOCKFREEHASHMAP_TEST_ALIGN_64 std::atomic<long long> LockFreeHashMapTests::s_liveAllocations(0);
LOCKFREEHASHMAP_TEST_ALIGN_64 std::atomic<long long> LockFreeHashMapTests::s_pauseAllocation(-1);
std::atomic<bool> LockFreeHashMapTests::s_allocationEntered(false);
std::atomic<bool> LockFreeHashMapTests::s_allocationResume(false);
std::atomic<int> LockFreeHashMapTestValue::s_live(0);
std::atomic<int> LockFreeHashMapTestValue::s_copies(0);
std::atomic<int> LockFreeHashMapTestValue::s_failCopy(-1);
std::atomic<int> LockFreeHashMapTestValue::s_pauseValue(-1);
std::atomic<bool> LockFreeHashMapTestValue::s_entered(false);
std::atomic<bool> LockFreeHashMapTestValue::s_resume(false);
std::atomic<int> LockFreeHashMapTestValue::s_pauseDestructor(-1);
std::atomic<bool> LockFreeHashMapTestValue::s_destructorTimedOut(false);
std::atomic<bool> LockFreeHashMapTestHash::s_fail(false);
std::atomic<bool> LockFreeHashMapTestEqual::s_fail(false);

/** 追踪本测试进程的实际申请，故障注入只属于测试程序
@param [in] size 请求字节数，先检查额外计账空间的溢出
@return 正确对齐的申请地址，失败抛 bad_alloc 供调用边界处理
*/
void* operator new(size_t size)
{
    long long countdown = LockFreeHashMapTests::s_tracking ? LockFreeHashMapTests::s_failAllocation.load() : -1;
    if (countdown >= 0 && LockFreeHashMapTests::s_failAllocation.fetch_sub(1) == 0)
    {
        throw std::bad_alloc();
    }
    if (LockFreeHashMapTests::s_tracking && LockFreeHashMapTests::s_pauseAllocation.load() >= 0 &&
        LockFreeHashMapTests::s_pauseAllocation.fetch_sub(1) == 0)
    {
        LockFreeHashMapTests::s_allocationEntered.store(true);
        if (!LockFreeHashMapTests::wait(LockFreeHashMapTests::s_allocationResume, 10000))
        {
            throw std::bad_alloc();
        }
    }
    if (size > (std::numeric_limits<size_t>::max)() - 32)
    {
        throw std::bad_alloc();
    }
    unsigned char* result = static_cast<unsigned char*>(std::malloc(size + 32));
    if (result == nullptr)
    {
        throw std::bad_alloc();
    }
    result[0] = LockFreeHashMapTests::s_tracking ? 1 : 0;
    if (result[0] != 0)
    {
        LockFreeHashMapTests::s_allocations.fetch_add(1);
        LockFreeHashMapTests::s_liveAllocations.fetch_add(1);
    }
    return result + 32;
}

/** 释放配对申请并精确撤销计账
@param [in] pointer 对应测试申请地址，可为空，载荷必须已析构
*/
// 计账头属于底层 malloc 块；不内联释放器，保留真实分配边界供内存检测器检查
#if defined(_MSC_VER)
__declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
void operator delete(void* pointer) throw()
{
    if (pointer != nullptr)
    {
        unsigned char* allocation = static_cast<unsigned char*>(pointer) - 32;
        if (allocation[0] != 0)
        {
            LockFreeHashMapTests::s_liveAllocations.fetch_sub(1);
        }
        std::free(allocation);
    }
}

/** 追踪本测试进程的实际申请，故障注入只属于测试程序
@param [in] size 请求字节数，先检查额外计账空间的溢出
@return 正确对齐的申请地址，失败抛 bad_alloc 供调用边界处理
*/
void* operator new[](size_t size)
{
    return ::operator new(size);
}

/** 释放配对申请并精确撤销计账
@param [in] pointer 对应测试申请地址，可为空，载荷必须已析构
*/
void operator delete[](void* pointer) throw()
{
    ::operator delete(pointer);
}

size_t LockFreeHashMapCollisionHash::operator()(int value) const
{
    (void)value;
    return 0;
}

LockFreeHashMapTestValue::LockFreeHashMapTestValue(int value) :
m_value(value)
{
    s_live.fetch_add(1);
}

LockFreeHashMapTestValue::LockFreeHashMapTestValue(const LockFreeHashMapTestValue& other) :
m_value(other.m_value)
{
    int index = s_copies.fetch_add(1);
    if (index == s_failCopy.load())
    {
        throw std::runtime_error("injected value copy");
    }
    int expected = other.m_value;
    if (s_pauseValue.compare_exchange_strong(expected, -1))
    {
        s_entered.store(true);
        if (!LockFreeHashMapTests::wait(s_resume, 10000))
        {
            throw std::runtime_error("pause deadline");
        }
    }
    s_live.fetch_add(1);
}

LockFreeHashMapTestValue::~LockFreeHashMapTestValue()
{
    int expected = m_value;
    if (s_pauseDestructor.compare_exchange_strong(expected, -1))
    {
        s_entered.store(true);
        if (!LockFreeHashMapTests::wait(s_resume, 10000))
        {
            s_destructorTimedOut.store(true);
        }
    }
    s_live.fetch_sub(1);
}

size_t LockFreeHashMapTestHash::operator()(int key) const
{
    (void)key;
    if (s_fail.load())
    {
        throw std::runtime_error("injected hash");
    }
    return 0;
}

bool LockFreeHashMapTestEqual::operator()(int left, int right) const
{
    if (s_fail.load())
    {
        throw std::runtime_error("injected equality");
    }
    return left == right;
}

void LockFreeHashMapTests::check(bool condition, const char* message)
{
    ++s_checks;
    if (!condition)
    {
        ++s_failures;
        s_failAllocation.store(-1);
        std::cout << "FAIL " << message << std::endl;
        // 不继续依赖已经失败的前置条件，也不以主动崩溃结束测试
        throw LockFreeHashMapTestCheckFailed;
    }
}

bool LockFreeHashMapTests::wait(const std::atomic<bool>& flag, int milliseconds)
{
    double begin = timestamp();
    while (!flag.load() && timestamp() - begin < double(milliseconds) / 1000)
    {
        LockFreeHashMapTests::pause(1);
    }
    return flag.load();
}

void LockFreeHashMapTests::pause(unsigned milliseconds)
{
#if defined(_WIN32)
    Sleep(milliseconds);
#else
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
#endif
}

void LockFreeHashMapTests::yieldThread()
{
#if defined(_WIN32)
    SwitchToThread();
#else
    std::this_thread::yield();
#endif
}

double LockFreeHashMapTests::timestamp()
{
#if defined(_WIN32)
    LARGE_INTEGER ticks = {}, frequency = {};
    if (QueryPerformanceCounter(&ticks) && QueryPerformanceFrequency(&frequency) && frequency.QuadPart != 0)
    {
        return double(ticks.QuadPart) / double(frequency.QuadPart);
    }
#endif
    return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

void LockFreeHashMapTests::warmRuntime()
{
    long long previous = s_liveAllocations.load();
    int stable = 0;
    for (int round = 0; round < 32; ++round)
    {
#if !defined(LOCKFREEHASHMAP_TEST_SINGLE_THREAD)
        {
            std::vector<LockFreeHashMapTestThread> threads;
            for (int i = 0; i < 8; ++i)
            {
                // 无参数工作函数借用本用例夹具，结果在连接线程后读取，无返回值
                threads.push_back(LockFreeHashMapTestThread([]()
                {
                    LockFreeHashMapTests::pause(2);
                }));
            }
            for (size_t i = 0; i < threads.size(); ++i)
            {
                threads[i].join();
            }
        }
#endif
        std::cout << "RUNTIME_CONTROL round=" << round << " live_allocations=" << s_liveAllocations.load() << std::endl;
        long long current = s_liveAllocations.load();
        stable = current == previous ? stable + 1 : 0;
        previous = current;
        if (stable == 8)
        {
            return;
        }
    }
    check(false, "runtime-only allocation baseline stabilizes before container measurements");
}

void LockFreeHashMapTests::pool()
{
    // 低位终态和最高地址位必须完整保留，CAS 失败必须写回实际观察值。
    LockFreeHashMapLink link;
    check(link.isLockFree() && link.load() == 0, "atomic link initial state");
    uintptr_t observed = 0;
    const uintptr_t allBits = (std::numeric_limits<uintptr_t>::max)();
    check(link.compareExchangeStrong(observed, allBits) && observed == 0 && link.load() == allBits,
        "atomic link successful publication");
    check(!link.compareExchangeStrong(observed, 7) && observed == allBits && link.load() == allBits,
        "atomic link failed CAS updates expected");
    link.store(3, std::memory_order_release);
    check(link.load(std::memory_order_acquire) == 3, "atomic link preserves terminal flags");
    typedef LockFreeHashMapPool<int> Pool;
    typedef LockFreeHashMapPoolSlot<int> Slot;
    LockFreeHashMapPoolConfig config;
    config.inlineScalarValues = false;
    config.initialCapacity = 2;
    config.maximumCapacity = 6;
    config.directoryCapacity = 8;
    // 固定请求两槽，核对成功增长计数和硬上限
    config.growPolicy = [](const LockFreeHashMapGrowContext& context)
    {
        (void)context;
        return size_t(2);
    };
    {
        LockFreeHashMap<int, int> configured(7, config);
        LockFreeHashMapPoolStats directMetrics = {};
        check(configured.status() == LockFreeHashMapOk && configured.bucketCount() == 8,
            "configured constructor preserves bucket normalization");
        configured.poolStats(directMetrics);
        check(directMetrics.capacity == 2 && directMetrics.successfulGrowCount == 0,
            "configured constructor does not allocate the default pool first");
        check(configured.set(1, 3) && configured.value(1) == 3, "configured constructor accepts operations");
        LockFreeHashMapPoolConfig invalid = config;
        invalid.maximumCapacity = -2;
        LockFreeHashMap<int, int> rejectedDirect(8, invalid);
        check(rejectedDirect.status() == LockFreeHashMapInvalidArgument, "configured constructor rejects invalid pool");
    }
    for (int32_t injection = 0; injection < 24; ++injection)
    {
        const long long beforeDirect = s_liveAllocations.load();
        s_failAllocation.store(injection);
        {
            LockFreeHashMap<int, int> configured(8, config);
            s_failAllocation.store(-1);
            check(configured.status() == LockFreeHashMapOk || configured.status() == LockFreeHashMapNoMemory,
                "configured constructor allocation failure has explicit status");
        }
        check(s_liveAllocations.load() == beforeDirect, "configured constructor failure releases partial resources");
    }
    Pool pool;
    check(pool.initialize(config) == LockFreeHashMapOk, "bitmap pool initialized");
    Slot* slots[6] = {};
    {
        LockFreeHashMapEpochGuard guard(pool.epoch());
        check(guard.record() != nullptr, "pool operation record available");
        LockFreeHashMapStatus status;
        for (int i = 0; i < 6; ++i)
        {
            slots[i] = pool.acquire(*guard.record(), status);
            check(slots[i] != nullptr && status == LockFreeHashMapOk, "pooled payload acquired");
            new (slots[i]->payload()) int(i);
            slots[i]->m_kind.store(1);
            if (i == 1)
            {
                check(pool.stats().capacity == 2 && pool.stats().successfulGrowCount == 0, "zero threshold grows only after exhaustion");
            }
        }
        check(pool.stats().capacity == 6 && pool.stats().successfulGrowCount == 2, "fixed growth publishes two batches");
        check(pool.acquire(*guard.record(), status) == nullptr && status == LockFreeHashMapCapacityLimit, "full pool returns capacity status");
        pool.retire(slots[0]);
        check(pool.reclaim() == 0 && pool.acquire(*guard.record(), status) == nullptr,
            "paused epoch retains retired payload and hard budget");
    }
    check(pool.reclaim() == 1, "retirement reclaimed after operation leaves");
    {
        LockFreeHashMapEpochGuard guard(pool.epoch());
        check(guard.record() != nullptr, "reuse record available");
        LockFreeHashMapStatus status;
        slots[0] = pool.acquire(*guard.record(), status);
        check(slots[0] != nullptr && pool.stats().capacity == 6, "reuse at hard maximum needs no growth");
        new (slots[0]->payload()) int(8);
        slots[0]->m_kind.store(1);
        for (int i = 0; i < 6; ++i)
        {
            pool.release(slots[i]);
        }
    }
    // 一个维护者连续观察整块空闲后释放，回收不能低于初始容量
    LockFreeHashMapShrinkPolicy shrink = [](const LockFreeHashMapShrinkContext& context)
    {
        (void)context;
        return LockFreeHashMapShrinkDecision(100, 2);
    };
    pool.setPolicies(config.growPolicy, shrink);
    pool.shrink();
    pause(5);
    pool.shrink();
    pool.shrink();
    check(pool.stats().capacity == 2 && pool.stats().budget == 2 && pool.stats().successfulGrowCount == 2,
        "multi-block shrink preserves floor and cumulative growth history");

    LockFreeHashMap<int, int> map;
    config.initialCapacity = 1;
    config.maximumCapacity = 1;
    check(map.configurePool(config).m_status == LockFreeHashMapOk && map.set(1, 2), "fixed map uses one merged key value slot");
    check(map.trySet(1, 3).m_status == LockFreeHashMapCapacityLimit && map.value(1) == 2, "full replacement preserves current version");
    check(map.erase(1) && map.empty() && map.set(2, 3), "full deletion and next allocation reclaim without a tombstone slot");
    map.clearAndFreeMemory();
    check(map.retainedNodeCount() == 0 && map.set(3, 4), "explicit free permits reuse");
    LockFreeHashMap<int, int> positive;
    config.initialCapacity = 1;
    config.maximumCapacity = 8;
    config.growFreeThreshold = 1;
    check(positive.configurePool(config).m_status == LockFreeHashMapOk && positive.set(1, 2), "positive threshold admission");
    LockFreeHashMapPoolStats metrics = {};
    positive.poolStats(metrics);
    check(metrics.capacity > 1, "positive threshold pre-growth");
    LockFreeHashMap<int, int> rejected;
    config.maximumCapacity = -2;
    check(rejected.configurePool(config).m_status == LockFreeHashMapInvalidArgument, "negative maximum refused");
    config.maximumCapacity = 0;
    check(rejected.configurePool(config).m_status == LockFreeHashMapInvalidArgument, "maximum below initial refused");
    config.maximumCapacity = 8;
    // 策略抛出可捕获异常，不允许偷偷改用默认扩容
    config.growPolicy = [](const LockFreeHashMapGrowContext&) -> size_t { throw std::runtime_error("growth"); };
    check(rejected.configurePool(config).m_status == LockFreeHashMapOk && rejected.set(1, 2), "pre-growth failure preserves granted slot");
    check(rejected.trySet(2, 3).m_status == LockFreeHashMapUserException, "demand growth reports callback failure");
    rejected.setPoolPolicies([](const LockFreeHashMapGrowContext&) { return size_t(0); }, config.shrinkPolicy);
    check(rejected.trySet(2, 3).m_status == LockFreeHashMapGrowthDeclined, "zero request is an explicit refusal");
    rejected.setPoolPolicies([](const LockFreeHashMapGrowContext&) { return (std::numeric_limits<size_t>::max)(); }, config.shrinkPolicy);
    check(rejected.set(2, 3), "huge request safely clipped");
    rejected.poolStats(metrics);
    check(metrics.capacity == 8 && metrics.reservedCapacity == 0, "growth and reservations obey maximum");
}

void LockFreeHashMapTests::model()
{
    LockFreeHashMap<int, int, LockFreeHashMapCollisionHash> map(7);
    std::map<int, int> model;
    uint32_t random = 0x5eed1234;
    check(map.bucketCount() == 8, "bucket normalization");
    LockFreeHashMap<int, int> invalid((std::numeric_limits<size_t>::max)());
    check(invalid.status() == LockFreeHashMapInvalidArgument, "bucket overflow refused");
    for (int step = 0; step < 10000; ++step)
    {
        random = random * 1664525U + 1013904223U;
        int key = int((random >> 8) % 47);
        int op = int(random % 13);
        if (op < 4)
        {
            check(map.set(key, step), "model set accepted");
            model[key] = step;
        }
        else if (op < 7)
        {
            bool expected = model.insert(std::make_pair(key, step)).second;
            check(map.insert(key, step) == expected, "model insert identity");
        }
        else if (op < 9)
        {
            bool expected = model.erase(key) != 0;
            check(map.erase(key) == expected, "model erase identity");
        }
        else if (op == 12)
        {
            map.clear();
            model.clear();
        }
        else
        {
            int actual = -1;
            bool found = map.get(key, actual);
            check(found == (model.count(key) != 0), "model found state");
            if (found)
            {
                check(actual == model[key], "model value");
            }
        }
        if (step % 100 == 0)
        {
            std::vector<std::pair<int, int>> pairs = map.toVector();
            std::map<int, int> actual(pairs.begin(), pairs.end());
            check(actual == model && pairs.size() == model.size() && map.size() == model.size(), "full collision independent set");
        }
    }
    map.clear();
    for (int i = 0; i < 128; ++i)
    {
        map.maintain(256);
    }
    check(map.retainedNodeCount() == 0, "clear recovers old generations");
    map[4] = 7;
    map[5] = map[4];
    check(int(map[5]) == 7 && map.count(4) == 1 && map.remove(4), "legacy proxies and aliases");
    // 下标是允许抛出异常的特例，容量失败不能静默成功，也不能覆盖原值。
    LockFreeHashMapPoolConfig limited;
    limited.initialCapacity = 1;
    limited.maximumCapacity = 1;
    LockFreeHashMap<int, int> limitedMap(16, limited);
    limitedMap[1] = 10;
    bool caught = false;
    try
    {
        limitedMap[1] = 20;
    }
    catch (const LockFreeHashMapAssignmentError& error)
    {
        caught = error.status() == LockFreeHashMapCapacityLimit;
    }
    check(caught && limitedMap.value(1) == 10 && limitedMap.size() == 1, "subscript capacity exception preserves old value");
    LockFreeHashMapResult refused = limitedMap.trySet(1, 20);
    check(refused.m_status == LockFreeHashMapCapacityLimit && !refused.m_committed, "ordinary setter still returns status");
    map[10] = map[9999];
    check(map.value(10, -1) == 0, "proxy missing source retains default semantics");
    // 来源快照分配失败必须抛出错误，不能把默认值写入目标。
    s_failAllocation.store(0);
    caught = false;
    try
    {
        map[10] = map[5];
    }
    catch (const LockFreeHashMapAssignmentError& error)
    {
        caught = error.status() == LockFreeHashMapNoMemory;
    }
    s_failAllocation.store(-1);
    check(caught && map.value(10, -1) == 0, "proxy source failure does not overwrite destination");
}

void LockFreeHashMapTests::versions()
{
    check(LockFreeHashMapMigration::run(), "compiled ProcessWork caches and registry migration");
    typedef LockFreeHashMap<int, int> Map;
    Map map;
    Map other;
    Map::Snapshot old;
    Map::Snapshot actual;
    check(map.insertIfAbsent(1, 10, old).m_committed, "first publication");
    check(map.insertIfAbsent(1, 20, actual).m_status == LockFreeHashMapExists && *actual.m_value == 10, "loser sees winner");
    check(other.set(1, 10) && other.eraseIf(1, old.m_version).m_status == LockFreeHashMapVersionMismatch, "cross-instance refusal");
    check(map.compareExchange(1, old.m_version, 10, &actual).m_committed, "same value gets new version");
    check(map.eraseIf(1, old.m_version).m_status == LockFreeHashMapVersionMismatch, "old equal value token refused");
    check(map.eraseIf(1, actual.m_version).m_committed && map.set(1, 10), "delete and reinsert");
    check(!map.compareExchange(1, actual.m_version, 20).m_committed, "reinsertion invalidates old identity");
    map.find(1, old);
    map.clear();
    check(map.set(1, 20) && !map.eraseIf(1, old.m_version).m_committed, "clear separates identities");
    check(*old.m_value == 10, "snapshot independent after clear");
    for (int i = 0; i < 5000; ++i)
    {
        check(map.set(1, i), "version churn accepted");
    }
    check(!map.eraseIf(1, old.m_version).m_committed && map.retainedNodeCount() < 128, "address reuse no old token and bounded history");
    // 非空输出反复接收新快照，旧引用仍独立存活；版本输入允许与输出为同一对象。
    Map::Snapshot reused;
    check(map.find(1, reused).m_status == LockFreeHashMapOk, "snapshot reuse fixture");
    Map::Snapshot held = reused;
    check(map.trySet(1, 6000, &reused).m_committed && *reused.m_value == 6000 && *held.m_value == 4999,
        "snapshot handoff keeps escaped old value");
    check(reused.m_value.use_count() == 1 && held.m_value.use_count() == 1, "snapshot handoff retains no temporary owners");
    check(map.compareExchange(1, reused.m_version, 6001, &reused).m_committed && *reused.m_value == 6001,
        "snapshot version input aliases output");
    const int* preserved = reused.m_value.get();
    uint64_t preservedVersion = reused.m_version.m_sequence;
    s_failAllocation.store(0);
    LockFreeHashMapResult failedCopy = map.find(1, reused);
    s_failAllocation.store(-1);
    check(failedCopy.m_status == LockFreeHashMapNoMemory && reused.m_value.get() == preserved &&
        reused.m_version.m_sequence == preservedVersion, "snapshot allocation failure preserves both output fields");
    check(map.take(1, reused).m_committed && *reused.m_value == 6001 && !map.contains(1), "take replaces populated output");
    check(map.set(1, 4999), "restore traversal fixture");
    // 布尔向量在 v120 没有 emplace_back，导出仍须保留这类合法模板实例。
    LockFreeHashMap<bool, bool> booleanMap;
    check(booleanMap.set(false, true) && booleanMap.set(true, false), "boolean export fixture");
    std::vector<bool> booleanKeys = booleanMap.keys();
    std::vector<bool> booleanValues = booleanMap.values();
    check(booleanKeys.size() == 2 && std::count(booleanKeys.begin(), booleanKeys.end(), true) == 1 &&
        booleanValues.size() == 2 && std::count(booleanValues.begin(), booleanValues.end(), true) == 1,
        "boolean key and value export remains supported");
    Map::Snapshot escaped;
    {
        Map temporary;
        temporary.insertIfAbsent(2, 77, escaped);
    }
    check(*escaped.m_value == 77 && other.eraseIf(1, escaped.m_version).m_status == LockFreeHashMapVersionMismatch, "snapshot outlives instance");
    int callbacks = 0;
    // key、value 只在回调内借用；回调执行本场景校验或异常注入，无返回值
    LockFreeHashMapResult walked = map.forEachResult([&](const int& key, const int& value)
    {
        (void)value;
        ++callbacks;
        map.erase(key);
        map.set(key, 44);
    });
    check(walked.m_status == LockFreeHashMapOk && callbacks == 1, "callback reentry no duplicate replay");
}

void LockFreeHashMapTests::exceptions()
{
    typedef LockFreeHashMap<int, LockFreeHashMapTestValue, LockFreeHashMapTestHash, LockFreeHashMapTestEqual> Map;
    {
        Map map;
        LockFreeHashMapTestValue a(10);
        LockFreeHashMapTestValue b(20);
        Map::Snapshot before;
        check(map.insertIfAbsent(1, a, before).m_committed, "exception fixture");
        LockFreeHashMapTestHash::s_fail.store(true);
        check(map.trySet(1, b).m_status == LockFreeHashMapUserException, "hash failure is explicit");
        LockFreeHashMapTestHash::s_fail.store(false);
        LockFreeHashMapTestEqual::s_fail.store(true);
        check(map.trySet(1, b).m_status == LockFreeHashMapUserException && !map.tryErase(1).m_committed, "equality failure never means absent");
        LockFreeHashMapTestEqual::s_fail.store(false);
        LockFreeHashMapTestValue::s_failCopy.store(LockFreeHashMapTestValue::s_copies.load());
        check(map.compareExchange(1, before.m_version, b).m_status == LockFreeHashMapUserException, "value construction failure keeps entry");
        LockFreeHashMapTestValue::s_failCopy.store(LockFreeHashMapTestValue::s_copies.load());
        check(map.take(1, before).m_status == LockFreeHashMapUserException && map.contains(1), "take copy failure does not delete");
        LockFreeHashMapTestValue::s_failCopy.store(-1);
        // key、value 只在回调内借用；回调执行本场景校验或异常注入，无返回值
        check(map.forEachResult([](const int& key, const LockFreeHashMapTestValue& value)
        {
            (void)key;
            (void)value;
            throw std::runtime_error("callback");
        }).m_status == LockFreeHashMapUserException, "callback exception reported");
        check(map.eraseIf(1, before.m_version).m_committed, "exception paths release protections");
        // 导出可跳过内部计数，但第二次复制失败仍须销毁第一份副本，不能留下部分结果。
        check(map.set(1, a) && map.set(2, b) && map.set(3, a), "partial export fixture initialized");
        int live = LockFreeHashMapTestValue::s_live.load();
        LockFreeHashMapTestValue::s_failCopy.store(LockFreeHashMapTestValue::s_copies.load() + 1);
        bool emptyValues = map.values().empty();
        LockFreeHashMapTestValue::s_failCopy.store(-1);
        check(emptyValues && LockFreeHashMapTestValue::s_live.load() == live && map.size() == 3,
            "partial value export failure releases constructed elements and preserves source");
        LockFreeHashMapTestValue::s_failCopy.store(LockFreeHashMapTestValue::s_copies.load() + 1);
        bool emptyPairs = map.toVector().empty();
        LockFreeHashMapTestValue::s_failCopy.store(-1);
        check(emptyPairs && LockFreeHashMapTestValue::s_live.load() == live && map.keys().size() == 3,
            "partial pair export failure releases constructed elements and preserves source");
        int calls = 0;
        // 对外遍历在第二次回调失败时仍精确报告第一次正常完成，不能复用不计数的导出模式。
        LockFreeHashMapResult partial = map.forEachResult([&calls](const int& key, const LockFreeHashMapTestValue& value)
        {
            (void)key;
            (void)value;
            if (++calls == 2)
            {
                throw 77;
            }
        });
        check(partial.m_status == LockFreeHashMapUserException && partial.m_count == 1 && calls == 2 &&
            map.values().size() == 3 && map.toVector().size() == 3,
            "public traversal retains completed count and exports recover after partial failures");
    }
    check(LockFreeHashMapTestValue::s_live.load() == 0, "all payloads destroyed exactly once");
    // 逐点覆盖构造、配置、更新及拥有式 take 到达的每次申请
    const std::string stable("stable");
    const std::string next("next");
    const std::string capture(300, 'x');
    // context 为本次池状态快照；返回该场景要求的增长数量或收缩决策
    const LockFreeHashMapGrowPolicy policy = [capture](const LockFreeHashMapGrowContext& context)
    {
        (void)context;
        return capture.empty() ? size_t(0) : size_t(8);
    };
    for (int injection = 0; injection < 8; ++injection)
    {
        long long beforePolicy = s_liveAllocations.load();
        s_failAllocation.store(injection);
        try
        {
            // context 为本次池状态快照；返回该场景要求的增长数量或收缩决策
            LockFreeHashMapGrowPolicy prepared = [capture](const LockFreeHashMapGrowContext& context)
            {
                (void)context;
                return capture.empty() ? size_t(0) : size_t(8);
            };
        }
        catch (const std::bad_alloc&)
        {
            // 准备拥有式回调属于 C++ 构造边界，此时尚未修改 map
        }
        s_failAllocation.store(-1);
        check(s_liveAllocations.load() == beforePolicy, "policy preparation failure releases holder and partial callable");
    }
    long long observedAllocations = 0;
    long long beforeLive = s_liveAllocations.load();
    for (int injection = -1; injection < 128; ++injection)
    {
        long long beforeAlloc = s_allocations.load();
        long long beforeIteration = s_liveAllocations.load();
        s_failAllocation.store(injection);
        {
            LockFreeHashMap<int, std::string> map(1);
            if (map.status() == LockFreeHashMapOk)
            {
                LockFreeHashMapPoolConfig config;
                config.initialCapacity = 0;
                config.maximumCapacity = 8;
                config.directoryCapacity = 4;
                LockFreeHashMapResult configured = map.configurePool(config);
                if (configured.m_status == LockFreeHashMapOk)
                {
                    map.setPoolPolicies(policy, LockFreeHashMapShrinkPolicy());
                    LockFreeHashMap<int, std::string>::Snapshot snapshot;
                    LockFreeHashMapResult write = map.insertIfAbsent(1, stable, snapshot);
                    if (write.m_committed)
                    {
                        // key、value 只在回调内借用；回调执行本场景校验或异常注入，无返回值
                        map.forEachResult([](const int& key, const std::string& value)
                        {
                            (void)key;
                            (void)value;

                        });
                        // Debug STL 可能在包装体之前为返回向量本身申请内存
                        try
                        {
                            map.keys();
                            map.values();
                            map.toVector();
                        }
                        catch (const std::bad_alloc&)
                        {
                            // 兼容返回对象构造异常向调用方传播，map 内容保持不变
                        }
                        map.compareExchange(1, snapshot.m_version, next, &snapshot);
                        map.take(1, snapshot);
                    }
                }
            }
        }
        s_failAllocation.store(-1);
        if (s_liveAllocations.load() != beforeIteration)
        {
            std::cout << "ALLOCATION_DELTA injection=" << injection << " delta=" << s_liveAllocations.load() - beforeIteration << std::endl;
        }
        long long calls = s_allocations.load() - beforeAlloc;
        if (injection == -1)
        {
            observedAllocations = calls;
        }
        check(s_liveAllocations.load() == beforeLive, "allocation failure rollback has no leaked allocations");
    }
    std::cout << "ALLOC_PATH points=" << observedAllocations << " injected=0..127" << std::endl;
    check(observedAllocations < 128, "every observed allocation point injected");
}

void LockFreeHashMapTests::contention()
{
    typedef LockFreeHashMap<int, int, LockFreeHashMapCollisionHash> Map;
    Map map(1);
    std::atomic<bool> start(false);
    std::atomic<int> wins(0);
    std::atomic<int> failures(0);
    std::vector<LockFreeHashMapTestThread> threads;
    for (int t = 0; t < 8; ++t)
    {
        // 无参数工作函数借用本用例夹具，结果在连接线程后读取，无返回值
        threads.push_back(LockFreeHashMapTestThread([&, t]()
        {
            wait(start);
            Map::Snapshot result;
            LockFreeHashMapResult r = map.insertIfAbsent(1, t, result);
            if (r.m_committed)
            {
                wins.fetch_add(1);
            }
            if ((r.m_status != LockFreeHashMapOk && r.m_status != LockFreeHashMapExists) || !result.m_value)
            {
                failures.fetch_add(1);
            }
        }));
    }
    start.store(true);
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    check(wins.load() == 1 && failures.load() == 0 && map.size() == 1, "same-key exactly one publisher");
    map.set(1, 0);
    threads.clear();
    for (int t = 0; t < 8; ++t)
    {
        // 无参数工作函数借用本用例夹具，结果在连接线程后读取，无返回值
        threads.push_back(LockFreeHashMapTestThread([&]()
        {
            for (int i = 0; i < 1500; ++i)
            {
                bool done = false;
                for (int retry = 0; retry < 200000 && !done; ++retry)
                {
                    Map::Snapshot current;
                    LockFreeHashMapResult read = map.find(1, current);
                    if (read.m_status != LockFreeHashMapOk)
                    {
                        failures.fetch_add(1);
                        break;
                    }
                    LockFreeHashMapResult write = map.compareExchange(1, current.m_version, *current.m_value + 1);
                    done = write.m_committed;
                    if (!done && write.m_status != LockFreeHashMapVersionMismatch && write.m_status != LockFreeHashMapReservationBusy)
                    {
                        failures.fetch_add(1);
                        break;
                    }
                }
                if (!done)
                {
                    failures.fetch_add(1);
                }
            }
        }));
    }
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    check(failures.load() == 0 && map.value(1) == 12000, "version CAS increments have no lost updates");
    wins.store(0);
    threads.clear();
    for (int t = 0; t < 8; ++t)
    {
        // 无参数工作函数借用本用例夹具，结果在连接线程后读取，无返回值
        threads.push_back(LockFreeHashMapTestThread([&]()
        {
            Map::Snapshot result;
            if (map.take(1, result).m_committed)
            {
                wins.fetch_add(1);
                if (*result.m_value != 12000)
                {
                    failures.fetch_add(1);
                }
            }
        }));
    }
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    check(wins.load() == 1 && map.empty() && failures.load() == 0, "request cancel/complete unique terminal take");
}

void LockFreeHashMapTests::paused()
{
    typedef LockFreeHashMap<int, LockFreeHashMapTestValue> Map;
    {
        Map map(1);
        LockFreeHashMapPoolConfig config;
        // 给暂停记录可能暂占的四槽申请缓存以及新旧载荷都留出明确预算
        config.initialCapacity = 8;
        config.maximumCapacity = 8;
        config.protectionRecordCount = 4;
        config.directoryCapacity = 4;
        map.configurePool(config);
        map.set(1, LockFreeHashMapTestValue(10));
        LockFreeHashMapTestValue::s_entered.store(false);
        LockFreeHashMapTestValue::s_resume.store(false);
        LockFreeHashMapTestValue::s_pauseValue.store(10);
        Map::Snapshot captured;
        LockFreeHashMapResult reader;
        // 无参数工作函数借用本用例夹具，结果在连接线程后读取，无返回值
        LockFreeHashMapTestThread paused([&]()
        {
            reader = map.find(1, captured);
        });
        LockFreeHashMapTestResume resumeOnExit(LockFreeHashMapTestValue::s_resume, paused);
        bool entered = wait(LockFreeHashMapTestValue::s_entered);
        check(entered, "reader paused after protected pointer validation");
        check(map.trySet(1, LockFreeHashMapTestValue(20)).m_committed, "writer progresses while reader paused");
        check(map.erase(1), "delete progresses while reader paused");
        for (int i = 0; i < 100; ++i)
        {
            map.maintain(64);
        }
        LockFreeHashMapPoolStats observed = {};
        map.poolStats(observed);
        check(observed.retired >= 1 && observed.capacity <= 8, "paused value remains retired within budget");
        check(map.set(2, LockFreeHashMapTestValue(30)), "unrelated entry progresses with paused reader");
        LockFreeHashMapTestValue::s_resume.store(true);
        paused.join();
        check(reader.m_status == LockFreeHashMapOk && captured.m_value->m_value == 10, "paused copy remains valid after deletion");
        map.clear();
        for (int i = 0; i < 100; ++i)
        {
            map.maintain(64);
        }
        check(map.retainedNodeCount() == 0, "paused reader release permits full reclamation");
    }
    {
        Map map(1);
        LockFreeHashMapTestValue::s_entered.store(false);
        LockFreeHashMapTestValue::s_resume.store(false);
        LockFreeHashMapTestValue::s_pauseValue.store(99);
        LockFreeHashMapResult delayed;
        // 无参数工作函数借用本用例夹具，结果在连接线程后读取，无返回值
        LockFreeHashMapTestThread creator([&]()
        {
            delayed = map.trySet(1, LockFreeHashMapTestValue(99));
        });
        LockFreeHashMapTestResume resumeOnExit(LockFreeHashMapTestValue::s_resume, creator);
        check(wait(LockFreeHashMapTestValue::s_entered), "candidate paused before publication");
        map.clear();
        check(map.set(1, LockFreeHashMapTestValue(77)), "new generation publishes past paused candidate");
        LockFreeHashMapTestValue::s_resume.store(true);
        creator.join();
        Map::Snapshot actual;
        check(map.find(1, actual).m_status == LockFreeHashMapOk && actual.m_value->m_value == 77, "late old generation cannot overwrite new");
    }
    {
        LockFreeHashMap<int, int, LockFreeHashMapCollisionHash> map(1);
        for (int i = 1; i <= 6; ++i)
        {
            map.set(i, i);
        }
        std::atomic<bool> entered(false);
        std::atomic<bool> resume(false);
        std::atomic<int> errors(0);
        LockFreeHashMapResult walked;
        // 无参数工作函数借用本用例夹具，结果在连接线程后读取，无返回值
        LockFreeHashMapTestThread reader([&]()
        {
            // key、value 只在回调内借用；回调执行本场景校验或异常注入，无返回值
            walked = map.forEachResult([&](const int& key, const int& value)
            {
                if (key == 3)
                {
                    entered.store(true);
                    if (!wait(resume, 10000))
                    {
                        errors.fetch_add(1);
                    }
                }
                if (key != value)
                {
                    errors.fetch_add(1);
                }
            });
        });
        LockFreeHashMapTestResume resumeOnExit(resume, reader);
        check(wait(entered), "traversal paused inside a protected interior-node callback");
        check(map.erase(4) && map.erase(3) && map.erase(2), "unlink predecessor current and successor during callback");
        for (int i = 0; i < 2000; ++i)
        {
            map.set(8, 8);
            map.erase(8);
            map.maintain(64);
        }
        resume.store(true);
        reader.join();
        check(walked.m_status == LockFreeHashMapOk && errors.load() == 0 && walked.m_count == 5,
            "whole-operation protection permits surviving successor after concurrent interior removal");
        map.clear();
        // 本夹具在暂停期间积累两千个退役值，位图预算必须覆盖这些值，不能依赖缩容偷偷全量回收
        map.maintain(64);
        map.maintain(64);
        check(map.retainedNodeCount() == 0, "all interior protections leave after traversal");
    }
    for (int initialKeys = 0; initialKeys < 2; ++initialKeys)
    {
        Map map(1);
        if (initialKeys != 0)
        {
            map.set(0, LockFreeHashMapTestValue(0));
        }
        LockFreeHashMapTestValue::s_entered.store(false);
        LockFreeHashMapTestValue::s_resume.store(false);
        LockFreeHashMapTestValue::s_pauseValue.store(99);
        LockFreeHashMapResult delayed;
        Map::Snapshot lateOutput;
        // 无参数工作函数借用本用例夹具，结果在连接线程后读取，无返回值
        LockFreeHashMapTestThread creator([&]()
        {
            delayed = map.insertIfAbsent(1, LockFreeHashMapTestValue(99), lateOutput);
        });
        LockFreeHashMapTestResume resumeOnExit(LockFreeHashMapTestValue::s_resume, creator);
        check(wait(LockFreeHashMapTestValue::s_entered), "candidate paused before a competing absent-insert");
        Map::Snapshot winner;
        check(map.insertIfAbsent(1, LockFreeHashMapTestValue(77), winner).m_committed,
            "competing absent-insert published");
        check(map.tryClear().m_committed,
            "clear completes or pauses after generation CAS with a nonempty original tail");
        map.maintain(1);
        LockFreeHashMapTestValue::s_resume.store(true);
        creator.join();
        std::cout << "PAUSED_INSERT initial_keys=" << initialKeys << " committed=" << delayed.m_committed << " status=" << delayed.m_status << std::endl;
        check(!delayed.m_committed && delayed.m_status == LockFreeHashMapCleared && !map.contains(1),
            "whole old bucket detach prevents two successful absent-inserts before the same clear");
    }
    {
        Map map(1);
        LockFreeHashMapPoolConfig config;
        config.initialCapacity = 5;
        config.maximumCapacity = 5;
        config.protectionRecordCount = 4;
        map.configurePool(config);
        map.set(1, LockFreeHashMapTestValue(10));
        LockFreeHashMapTestValue::s_entered.store(false);
        LockFreeHashMapTestValue::s_resume.store(false);
        LockFreeHashMapTestValue::s_pauseDestructor.store(10);
        LockFreeHashMapResult published;
        // 无参数工作函数借用本用例夹具，结果在连接线程后读取，无返回值
        LockFreeHashMapTestThread reclaimer([&]()
        {
            published = map.trySet(1, LockFreeHashMapTestValue(20));
            map.maintain(1);
        });
        LockFreeHashMapTestResume resumeOnExit(LockFreeHashMapTestValue::s_resume, reclaimer);
        check(wait(LockFreeHashMapTestValue::s_entered), "writer paused after publication while destroying detached old value");
        Map::Snapshot visible;
        check(map.find(1, visible).m_status == LockFreeHashMapOk && visible.m_value->m_value == 20,
            "published replacement visible before paused writer returns");
        check(map.erase(1) && map.set(2, LockFreeHashMapTestValue(30)), "other operations pass a paused reclamation owner");
        LockFreeHashMapTestValue::s_resume.store(true);
        reclaimer.join();
        check(published.m_committed && !LockFreeHashMapTestValue::s_destructorTimedOut.load(), "reclaimer resumes and frees its private retired slot");
    }
    check(LockFreeHashMapTestValue::s_live.load() == 0, "paused scenarios release payloads");
}

bool LockFreeHashMapTests::linearize(const std::vector<LockFreeHashMapHistory>& history, unsigned used, std::map<int, int> state)
{
    if (used == (1U << history.size()) - 1)
    {
        return true;
    }
    for (size_t i = 0; i < history.size(); ++i)
    {
        if (used & (1U << i))
        {
            continue;
        }
        bool blocked = false;
        for (size_t j = 0; j < history.size(); ++j)
        {
            if (!(used & (1U << j)) && history[j].m_end < history[i].m_begin)
            {
                blocked = true;
            }
        }
        if (blocked)
        {
            continue;
        }
        const LockFreeHashMapHistory& operation = history[i];
        std::map<int, int> next = state;
        bool valid = false;
        bool exists = state.count(operation.m_key) != 0;
        if (operation.m_operation == 0)
        {
            valid = operation.m_committed && operation.m_status == LockFreeHashMapOk;
            next[operation.m_key] = operation.m_input;
        }
        else if (operation.m_operation == 1)
        {
            valid = exists ? (!operation.m_committed && operation.m_status == LockFreeHashMapExists && operation.m_output == state[operation.m_key]) :
                (operation.m_committed && operation.m_status == LockFreeHashMapOk);
            if (!exists)
            {
                next[operation.m_key] = operation.m_input;
            }
        }
        else if (operation.m_operation == 2)
        {
            valid = operation.m_committed == exists && operation.m_status == (exists ? LockFreeHashMapOk : LockFreeHashMapNotFound);
            next.erase(operation.m_key);
        }
        else if (operation.m_operation == 3)
        {
            valid = !operation.m_committed && operation.m_status == (exists ? LockFreeHashMapOk : LockFreeHashMapNotFound) &&
                (!exists || operation.m_output == state[operation.m_key]);
        }
        else
        {
            valid = operation.m_committed && operation.m_status == LockFreeHashMapOk;
            next.clear();
        }
        // Cleared 明确表示本次未接纳，不产生模型状态转换
        if (operation.m_status == LockFreeHashMapCleared)
        {
            bool overlapsClear = false;
            for (size_t j = 0; j < history.size(); ++j)
            {
                overlapsClear = overlapsClear || (history[j].m_operation == 4 && history[j].m_begin < operation.m_end &&
                    history[j].m_end > operation.m_begin);
            }
            valid = !operation.m_committed && operation.m_operation < 3 && overlapsClear;
            next = state;
        }
        if (valid && linearize(history, used | (1U << i), next))
        {
            return true;
        }
    }
    return false;
}

void LockFreeHashMapTests::histories()
{
    uint32_t random = 0x1234abcd;
    for (int round = 0; round < 600; ++round)
    {
        LockFreeHashMap<int, int, LockFreeHashMapCollisionHash> map(1);
        if (round >= 200 && round < 400)
        {
            LockFreeHashMapTestAccess::historySequence(map, round % 2 == 0 ? UINT64_C(0x7ffffffd) : UINT64_C(0xfffffffd));
        }
        if (round >= 400)
        {
            LockFreeHashMapPoolConfig config;
            config.inlineScalarValues = round < 500;
            config.lookupCacheCapacity = round % 2 == 0 ? 1 : 256;
            map.configurePool(config);
        }
        std::vector<LockFreeHashMapHistory> history(12);
        for (size_t i = 0; i < history.size(); ++i)
        {
            random = random * 1664525U + 1013904223U;
            history[i].m_operation = int((random >> 12) % 5);
            history[i].m_key = int(random % 2);
            history[i].m_input = int(i + 1);
        }
        std::atomic<int> clock(0);
        std::atomic<bool> start(false);
        std::vector<LockFreeHashMapTestThread> threads;
        try
        {
            for (int t = 0; t < 3; ++t)
            {
                // 无参数工作函数借用本用例夹具，结果在连接线程后读取，无返回值
                threads.push_back(LockFreeHashMapTestThread([&, t]()
                {
                    wait(start);
                    for (int j = 0; j < 4; ++j)
                    {
                        LockFreeHashMapHistory& item = history[t * 4 + j];
                        item.m_begin = clock.fetch_add(1);
                        LockFreeHashMapSnapshot<int> output;
                        LockFreeHashMapResult result;
                        int directOutput = -1;
                        bool directRead = round >= 400 && item.m_operation == 3;
                        if (item.m_operation == 0)
                        {
                            result = map.trySet(item.m_key, item.m_input);
                        }
                        if (item.m_operation == 1)
                        {
                            result = map.insertIfAbsent(item.m_key, item.m_input, output);
                        }
                        if (item.m_operation == 2)
                        {
                            result = map.tryErase(item.m_key);
                        }
                        if (item.m_operation == 3)
                        {
                            if (directRead)
                            {
                                result = LockFreeHashMapResult(map.get(item.m_key, directOutput) ? LockFreeHashMapOk : LockFreeHashMapNotFound);
                            }
                            else
                            {
                                result = map.find(item.m_key, output);
                            }
                        }
                        if (item.m_operation == 4)
                        {
                            result = map.tryClear();
                        }
                        item.m_status = result.m_status;
                        item.m_committed = result.m_committed;
                        item.m_output = directRead ? directOutput : (output.m_value ? *output.m_value : -1);
                        item.m_end = clock.fetch_add(1);
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
        for (size_t i = 0; i < threads.size(); ++i)
        {
            threads[i].join();
        }
        bool valid = linearize(history, 0, std::map<int, int>());
        if (!valid)
        {
            for (size_t i = 0; i < history.size(); ++i)
            {
                const LockFreeHashMapHistory& h = history[i];
                std::cout << "HISTORY " << round << ' ' << i << ' ' << h.m_operation << ' ' << h.m_key << ' ' << h.m_input << ' '
                    << h.m_begin << ' ' << h.m_end << ' ' << h.m_status << ' ' << h.m_committed << ' ' << h.m_output << std::endl;
            }
        }
        check(valid, "history has a legal real-time sequential order");
    }
}

void LockFreeHashMapTests::stress()
{
    LockFreeHashMap<int, int, LockFreeHashMapCollisionHash> map(1);
    LockFreeHashMapPoolConfig config;
    config.initialCapacity = 512;
    config.maximumCapacity = 2048;
    config.directoryCapacity = 16;
    // context 为本次池状态快照；返回该场景要求的增长数量或收缩决策
    config.growPolicy = [](const LockFreeHashMapGrowContext& context)
    {
        (void)context;
        return size_t(128);
    };
    // context 为本次池状态快照；返回该场景要求的增长数量或收缩决策
    config.shrinkPolicy = [](const LockFreeHashMapShrinkContext& context)
    {
        (void)context;
        return LockFreeHashMapShrinkDecision(128, 2);
    };
    map.configurePool(config);
    std::atomic<int> failures(0);
    std::atomic<bool> finished(false);
    // 各工作者只在错误路径写自己的诊断格，避免在正常路径增加共享计数而掩盖交错
    uint64_t refused[6][16] = {};
    uint64_t readMissing[6] = {}, readWrong[6] = {}, eraseFailed[6] = {};
    uint64_t completed[6] = {}, refusedChanged[6] = {};
    uint64_t maintenanceFailed[3] = {};
    // 无参数工作函数借用本用例夹具，结果在连接线程后读取，无返回值
    LockFreeHashMapTestThread maintenance([&]()
    {
        while (!finished.load())
        {
            map.maintain(64);
            LockFreeHashMapPoolStats metrics = {};
            map.poolStats(metrics);
            if (metrics.budget > 2048 || metrics.peakBudget > 2048)
            {
                ++maintenanceFailed[0];
                failures.fetch_add(1);
            }
            // key、value 只在回调内借用；回调执行本场景校验或异常注入，无返回值
            LockFreeHashMapResult walk = map.forEachResult([&](const int& key, const int& value)
            {
                if (key < 0 || key >= 96 || value < 0 || value >= 10000)
                {
                    ++maintenanceFailed[1];
                    failures.fetch_add(1);
                }
            });
            if (walk.m_status != LockFreeHashMapOk)
            {
                ++maintenanceFailed[2];
                failures.fetch_add(1);
            }
            LockFreeHashMapTests::yieldThread();
        }
    });
    std::vector<LockFreeHashMapTestThread> threads;
    try
    {
        for (int t = 0; t < 6; ++t)
        {
            // 无参数工作函数借用本用例夹具，只写自身键区间，并累计失败次数
            threads.push_back(LockFreeHashMapTestThread([&, t]()
            {
                uint64_t committed = 0;
                for (int i = 0; i < 10000; ++i)
                {
                    int key = t * 16 + i % 16;
                    LockFreeHashMapResult r;
                    size_t attempts = 0;
                    do
                    {
                        r = map.trySet(key, i);
                        if (r.m_committed)
                        {
                            break;
                        }
                        int32_t status = static_cast<int32_t>(r.m_status);
                        if (status >= 0 && status < 16)
                        {
                            ++refused[t][status];
                        }
                        // 有限池允许暂时拒绝；拒绝必须不改旧值，并且不能吞掉本次逻辑写入
                        int oldValue = -1;
                        bool oldFound = map.get(key, oldValue);
                        bool expectedFound = i >= 16 && (i - 16) % 3 != 0;
                        if (oldFound != expectedFound || (oldFound && oldValue != i - 16))
                        {
                            ++refusedChanged[t];
                            break;
                        }
                        if (r.m_status != LockFreeHashMapCapacityLimit && r.m_status != LockFreeHashMapReservationBusy)
                        {
                            break;
                        }
                        LockFreeHashMapTests::yieldThread();
                    } while (++attempts < 10000);
                    if (!r.m_committed)
                    {
                        failures.fetch_add(1);
                        break;
                    }
                    ++committed;
                    int value = -1;
                    bool found = map.get(key, value);
                    if (!found || value != i)
                    {
                        ++(found ? readWrong[t] : readMissing[t]);
                        failures.fetch_add(1);
                    }
                    if (i % 3 == 0 && !map.erase(key))
                    {
                        ++eraseFailed[t];
                        failures.fetch_add(1);
                    }
                }
                completed[t] = committed;
            }));
        }
    }
    catch (...)
    {
        // 后续工作线程创建失败也先停止维护者，再由作用域线程所有者收拢已启动线程
        finished.store(true);
        throw;
    }
    for (size_t i = 0; i < threads.size(); ++i)
    {
        threads[i].join();
    }
    finished.store(true);
    maintenance.join();
    uint64_t committed = 0;
    for (size_t thread = 0; thread < 6; ++thread)
    {
        for (size_t status = 0; status < 16; ++status)
        {
            if (refused[thread][status] != 0)
            {
                std::cout << "STRESS_REFUSED thread=" << thread << " status=" << status << " count=" << refused[thread][status] << std::endl;
            }
        }
        std::cout << "STRESS_DATA thread=" << thread << " missing=" << readMissing[thread] << " wrong=" << readWrong[thread]
            << " erase_failed=" << eraseFailed[thread] << " refused_changed=" << refusedChanged[thread]
            << " completed=" << completed[thread] << std::endl;
        committed += completed[thread];
    }
    std::cout << "STRESS_MAINTENANCE budget=" << maintenanceFailed[0] << " range=" << maintenanceFailed[1]
        << " status=" << maintenanceFailed[2] << std::endl;
    check(failures.load() == 0, "full collision concurrent churn exact per-owner values");
    check(committed == 60000, "finite pool refusals preserve all sixty thousand logical writes");
    map.clear();
    for (int i = 0; i < 300; ++i)
    {
        map.maintain(512);
    }
    check(map.retainedNodeCount() == 0 && map.empty(), "stress complete resource reclamation");
    LockFreeHashMapPoolStats metrics = {};
    map.poolStats(metrics);
    std::cout << "STRESS operations=60000 threads=6 collision=all capacity=" << metrics.capacity
        << " retained=" << metrics.occupied << " bytes=" << metrics.retainedBytes << " cas_retries=NOT_MEASURED" << std::endl;
}

void LockFreeHashMapTests::performance()
{
    for (int mode = 0; mode < 3; ++mode)
    {
        size_t buckets = mode == 0 ? 1024 : (mode == 1 ? 8 : 1);
        LockFreeHashMap<int, int> map(buckets);
        std::vector<double> latency;
        latency.reserve(10000);
        double start = timestamp();
        for (int i = 0; i < 10000; ++i)
        {
            double begin = timestamp();
            bool ok = map.set(i % 64, i);
            int value = -1;
            ok = ok && map.get(i % 64, value) && value == i;
            check(ok, "benchmark data correctness");
            latency.push_back((timestamp() - begin) * 1000000);
        }
        double seconds = timestamp() - start;
        std::sort(latency.begin(), latency.end());
        LockFreeHashMapPoolStats metrics = {};
        map.poolStats(metrics);
        std::cout << "PERF buckets=" << buckets << " pairs_per_second=" << 10000 / seconds << " p50_us=" << latency[5000]
            << " p95_us=" << latency[9500] << " p99_us=" << latency[9900] << " capacity=" << metrics.capacity
            << " retained=" << metrics.occupied << " bytes=" << metrics.retainedBytes << " system_allocations=" << metrics.systemAllocationCount << std::endl;
    }
}

int LockFreeHashMapRunTests(int argc, char** argv)
{
    LockFreeHashMapTests::s_tracking = true;
    int runningId = 0;
    try
    {
        LockFreeHashMapTests::warmRuntime();
        LockFreeHashMapTests::check(LockFreeHashMapTestThread::failures() == 0,
            "runtime warmup worker exceptions are not ignored");
        char* end = nullptr;
        long requested = argc == 2 ? std::strtol(argv[1], &end, 10) : 0;
        if (argc > 2 || requested < 0 || requested > 10 ||
            (argc == 2 && (requested == 0 || end == argv[1] || *end != 0)))
        {
            std::cerr << "invalid test ID; choose 1..10 or omit for all" << std::endl;
            return 2;
        }
        int selected = static_cast<int>(requested);
        for (int id = 1; id <= 10; ++id)
        {
            if (selected != 0 && selected != id)
            {
                continue;
            }
#if defined(LOCKFREEHASHMAP_TEST_SINGLE_THREAD)
            if (id != 1 && id != 2 && id != 3 && id != 4 && id != 9)
            {
                std::cout << "NOT_RUN id=" << id << " reason=single_thread_host" << std::endl;
                continue;
            }
#endif
            int before = LockFreeHashMapTests::s_failures;
            int threadFailuresBefore = LockFreeHashMapTestThread::failures();
            runningId = id;
            std::cout << "BEGIN id=" << id << std::endl;
            long long allocationsBefore = LockFreeHashMapTests::s_liveAllocations.load();
            LockFreeHashMapTestResources resourcesBefore;
            if (id == 1)
            {
                LockFreeHashMapTests::pool();
            }
            if (id == 2)
            {
                LockFreeHashMapTests::model();
            }
            if (id == 3)
            {
                LockFreeHashMapTests::versions();
            }
            if (id == 4)
            {
                LockFreeHashMapTests::exceptions();
            }
            if (id == 5)
            {
                LockFreeHashMapTests::contention();
            }
            if (id == 6)
            {
                LockFreeHashMapTests::paused();
            }
            if (id == 7)
            {
                LockFreeHashMapTests::histories();
            }
            if (id == 8)
            {
                LockFreeHashMapTests::stress();
            }
            if (id == 9)
            {
                LockFreeHashMapTests::performance();
            }
            if (id == 10)
            {
                LockFreeHashMapTests::boundaries();
            }
            LockFreeHashMapTestResources resourcesAfter;
            LockFreeHashMapTests::check(LockFreeHashMapTestThread::failures() == threadFailuresBefore,
                "all worker functions and thread joins completed without hidden exceptions");
            LockFreeHashMapTests::check(LockFreeHashMapTests::s_liveAllocations.load() == allocationsBefore,
                "test releases every tracked allocation");
#if defined(_WIN32)
            LockFreeHashMapTests::check(resourcesAfter.m_handles != 0 && resourcesAfter.m_threads != 0,
                "native process observations available; compare with independent runtime control");
#endif
            std::cout << "RESOURCES handles_before=" << resourcesBefore.m_handles << " handles_after=" << resourcesAfter.m_handles
                << " threads_before=" << resourcesBefore.m_threads << " threads_after=" << resourcesAfter.m_threads
                << " peak_working_set=" << resourcesAfter.m_peakWorkingSet
                << " allocation_delta=" << LockFreeHashMapTests::s_liveAllocations.load() - allocationsBefore
                << " cpu_seconds=" << resourcesAfter.m_cpuSeconds - resourcesBefore.m_cpuSeconds << std::endl;
            std::cout << "END id=" << id << " result=" << (before == LockFreeHashMapTests::s_failures ? "PASS" : "FAIL") << std::endl;
        }
        std::cout << "RESULT checks=" << LockFreeHashMapTests::s_checks << " failed=" << LockFreeHashMapTests::s_failures << std::endl;
        return LockFreeHashMapTests::s_failures == 0 ? 0 : 1;
    }
    catch (LockFreeHashMapTestFailure)
    {
        LockFreeHashMapTests::s_failAllocation.store(-1);
        std::cerr << "END id=" << runningId << " result=FAIL" << std::endl;
        std::cerr << "RESULT checks=" << LockFreeHashMapTests::s_checks.load()
            << " failed=" << LockFreeHashMapTests::s_failures.load() << std::endl;
        return 1;
    }
    catch (const std::exception& error)
    {
        LockFreeHashMapTests::s_failAllocation.store(-1);
        std::cerr << "UNEXPECTED " << error.what() << std::endl;
    }
    catch (...)
    {
        LockFreeHashMapTests::s_failAllocation.store(-1);
        std::cerr << "UNEXPECTED nonstandard exception" << std::endl;
    }
    return 3;
}