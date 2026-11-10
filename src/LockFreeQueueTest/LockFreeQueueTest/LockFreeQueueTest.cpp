#include "LockFreeQueueTest.h"
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstdlib>
#include <set>
#include "LockFreeQueueThreadGroup.h"
#include "LockFreeQueueBenchmarkPush.h"
#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>
#pragma comment(lib, "Psapi.lib")
#endif

#ifdef _MSC_VER
__declspec(thread) int32_t g_lockFreeQueueTestLane = 0;
#else
thread_local int32_t g_lockFreeQueueTestLane = 0;
#endif

#define LFQ_CHECK(expression)                                                                                          \
    if (!(expression))                                                                                                 \
    {                                                                                                                  \
        std::cout << "CHECK_FAIL line=" << __LINE__ << " expression=" << #expression << std::endl;                     \
        return false;                                                                                                  \
    }

std::atomic<int32_t> LockFreeQueueTestControl::m_pausePoint(0);
std::atomic<int32_t> LockFreeQueueTestControl::m_failPoint(0);
std::atomic<bool> LockFreeQueueTestControl::m_reached(false);
std::atomic<bool> LockFreeQueueTestControl::m_release(false);
std::atomic<bool> LockFreeQueueTestControl::m_timedOut(false);
std::atomic<uint64_t> LockFreeQueueTestControl::m_clock(UINT64_MAX);
std::atomic<size_t> LockFreeQueueTestControl::m_reclaimVisits(0);
std::atomic<size_t> LockFreeQueueTestControl::m_maintenanceVisits(0);
std::atomic<int32_t> LockFreeQueueTestControl::m_transientFailures(0);
std::atomic<size_t> LockFreeQueueTestControl::m_scheduleYields(0);
std::atomic<size_t> LockFreeQueueTestControl::m_scheduleSleeps(0);
bool LockFreeQueueTestAligned::m_valid = true;
std::atomic<int32_t> LockFreeQueueTestValue::m_live(0);
bool LockFreeQueueTestValue::m_throwCopy = false;
bool LockFreeQueueTestValue::m_throwAssign = false;
bool LockFreeQueueTestPolicy::m_throwCopy = false;

uint64_t LockFreeQueueTestControl::ticks()
{
#ifdef _WIN32
    LARGE_INTEGER counter;
    LARGE_INTEGER frequency;
    QueryPerformanceCounter(&counter);
    QueryPerformanceFrequency(&frequency);
    return static_cast<uint64_t>((counter.QuadPart / frequency.QuadPart) * 1000000000ULL +
                                 (counter.QuadPart % frequency.QuadPart) * 1000000000ULL / frequency.QuadPart);
#else
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
            .count());
#endif
}

uint64_t LockFreeQueueTestNow(uint64_t actual)
{
    const uint64_t injected = LockFreeQueueTestControl::m_clock.load();
    return injected == UINT64_MAX ? actual : injected;
}

uint32_t LockFreeQueueTestControl::handles()
{
#ifdef _WIN32
    DWORD count = 0;
    return GetProcessHandleCount(GetCurrentProcess(), &count) ? count : 0;
#else
    return 0;
#endif
}

uint64_t LockFreeQueueTestControl::privateBytes()
{
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS_EX memory = {};
    memory.cb = sizeof(memory);
    return GetProcessMemoryInfo(
               GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory))
               ? memory.PrivateUsage
               : 0;
#else
    return 0;
#endif
}

uint32_t LockFreeQueueTestControl::settledHandles()
{
    uint32_t observed = handles();
    const uint64_t deadline = ticks() + 5000000000ULL;
    uint64_t unchangedSince = ticks();
    while (ticks() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        const uint32_t current = handles();
        if (current != observed)
        {
            observed = current;
            unchangedSince = ticks();
        }
        if (ticks() - unchangedSince >= 1500000000ULL)
        {
            return observed;
        }
    }
    return UINT32_MAX;
}

LockFreeQueueTestAligned::LockFreeQueueTestAligned(int32_t value) : m_value(value)
{
    m_valid = m_valid && reinterpret_cast<uintptr_t>(this) % 64 == 0;
}

LockFreeQueueTestAligned::LockFreeQueueTestAligned(const LockFreeQueueTestAligned& other) : m_value(other.m_value)
{
    m_valid = m_valid && reinterpret_cast<uintptr_t>(this) % 64 == 0;
}

LockFreeQueueTestAligned& LockFreeQueueTestAligned::operator=(const LockFreeQueueTestAligned& other)
{
    m_value = other.m_value;
    return *this;
}

void LockFreeQueueTestControl::reset()
{
    m_pausePoint.store(0);
    m_failPoint.store(0);
    m_reached.store(false);
    m_release.store(false);
    m_timedOut.store(false);
    m_clock.store(UINT64_MAX);
    m_reclaimVisits.store(0);
    m_maintenanceVisits.store(0);
    m_transientFailures.store(0);
    m_scheduleYields.store(0);
    m_scheduleSleeps.store(0);
}

/** 仅隔离测试副本记录真实调度调用，不替换 yield/sleep
@param [in] sleeping true 表示正时长休眠，false 表示 yield
*/
void LockFreeQueueTestSchedule(bool sleeping)
{
    (sleeping ? LockFreeQueueTestControl::m_scheduleSleeps : LockFreeQueueTestControl::m_scheduleYields).fetch_add(1);
}

/** 隔离测试副本在真正提交之前注入一次暂时失败
@return true本次未提交，false执行真实队列算法
*/
bool LockFreeQueueTestTransient()
{
    int32_t remaining = LockFreeQueueTestControl::m_transientFailures.load();
    while (remaining > 0)
    {
        if (LockFreeQueueTestControl::m_transientFailures.compare_exchange_weak(remaining, remaining - 1))
        {
            return true;
        }
    }
    return false;
}

/** 只在指定测试点令真实 CAS 的 expected 失配
@param [in] point 0 为旧发布注入，31 为登记，32 为节点位图
@return 是否消耗一次竞争注入
*/
bool LockFreeQueueTestCas(int32_t point)
{
    return LockFreeQueueTestControl::m_failPoint.load() == point && LockFreeQueueTestTransient();
}

bool LockFreeQueueTestControl::waitReached()
{
    const uint64_t deadline = ticks() + 3000000000ULL;
    while (!m_reached.load() && ticks() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return m_reached.load();
}

/** Test-only scheduling/allocation boundary; production does not reference this symbol
@param [in] point Stable point: 1/2/3/4 queue, 5/6/7/8 pool, 10/11/12 allocation
@param [in] owner Diagnostic owner address, never dereferenced
*/
/** 在指定发布完成点暂停单个测试线程，超时记录失败
@param [in] lane 0 至 7 的测试生产者下标
*/
static void LockFreeQueuePausePublisher(size_t lane);
void LockFreeQueueTestHook(int32_t point, const void* owner)
{
    if (point == 3 && g_lockFreeQueueTestLane >= 100)
    {
        LockFreeQueuePausePublisher(static_cast<size_t>(g_lockFreeQueueTestLane - 100));
        g_lockFreeQueueTestLane = 0;
        return;
    }
    (void)owner;
    if (point == 20)
    {
        LockFreeQueueTestControl::m_reclaimVisits.fetch_add(1);
    }
    if (point == 24)
    {
        LockFreeQueueTestControl::m_maintenanceVisits.fetch_add(1);
    }
    int32_t fail = point;
    if (LockFreeQueueTestControl::m_failPoint.compare_exchange_strong(fail, 0))
    {
        throw std::bad_alloc();
    }
    if (g_lockFreeQueueTestLane != 1)
    {
        return;
    }
    int32_t pause = point;
    if (!LockFreeQueueTestControl::m_pausePoint.compare_exchange_strong(pause, 0))
    {
        return;
    }
    LockFreeQueueTestControl::m_reached.store(true);
    const uint64_t deadline = LockFreeQueueTestControl::ticks() + 5000000000ULL;
    while (!LockFreeQueueTestControl::m_release.load() && LockFreeQueueTestControl::ticks() < deadline)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (!LockFreeQueueTestControl::m_release.load())
    {
        LockFreeQueueTestControl::m_timedOut.store(true);
    }
}

LockFreeQueueTestValue::LockFreeQueueTestValue(int32_t value) : m_value(value)
{
    ++m_live;
}

LockFreeQueueTestValue::LockFreeQueueTestValue(const LockFreeQueueTestValue& other) : m_value(other.m_value)
{
    if (m_throwCopy)
    {
        throw std::runtime_error("copy injected");
    }
    ++m_live;
}

LockFreeQueueTestValue& LockFreeQueueTestValue::operator=(const LockFreeQueueTestValue& other)
{
    m_value = -999;
    if (m_throwAssign)
    {
        throw std::runtime_error("assignment injected after partial output modification");
    }
    m_value = other.m_value;
    return *this;
}

LockFreeQueueTestValue::~LockFreeQueueTestValue()
{
    --m_live;
}

LockFreeQueueTestThreads::~LockFreeQueueTestThreads()
{
    join();
}

void LockFreeQueueTestThreads::join()
{
    for (size_t i = 0; i < m_threads.size(); ++i)
    {
        if (m_threads[i].joinable())
        {
            m_threads[i].join();
        }
    }
}

LockFreeQueueTestPolicy::LockFreeQueueTestPolicy()
{
}

LockFreeQueueTestPolicy::LockFreeQueueTestPolicy(const LockFreeQueueTestPolicy& other)
{
    (void)other;
    if (m_throwCopy)
    {
        throw std::bad_alloc();
    }
}

size_t LockFreeQueueTestPolicy::operator()(const LockFreeQueueGrowContext& context) const
{
    (void)context;
    return 2;
}

bool LockFreeQueueTests::configure(
    LockFreeQueue<int32_t>& queue, size_t initial, int64_t maximum, size_t threshold, size_t producers)
{
    queue.destroy();
    LockFreeQueuePoolConfig config;
    config.initialCapacity = initial;
    config.maximumCapacity = maximum;
    config.growThreshold = threshold;
    config.maximumProducers = producers;
    return queue.configurePool(config) == LockFreeQueueSuccess && queue.tryInit() == LockFreeQueueSuccess;
}

bool LockFreeQueueTests::atomicValues()
{
    LockFreeQueueAtomic<uint32_t> word(UINT32_MAX);
    LockFreeQueueAtomic<uint64_t> wide(UINT64_C(0x123456789abcdef0));
    int32_t first = 1, second = 2;
    LockFreeQueueAtomic<int32_t*> pointer(&first);
    LFQ_CHECK(word.is_lock_free() && wide.is_lock_free() && pointer.is_lock_free());
    uint64_t wrong = 0;
    LFQ_CHECK(!wide.compare_exchange_strong(wrong, 9, std::memory_order_acq_rel, std::memory_order_acquire) &&
              wrong == UINT64_C(0x123456789abcdef0));
    LFQ_CHECK(wide.compare_exchange_strong(wrong, UINT64_MAX, std::memory_order_release));
    LFQ_CHECK(wide.exchange(0, std::memory_order_acquire) == UINT64_MAX && wide.load() == 0);
    int32_t* expected = &second;
    LFQ_CHECK(!pointer.compare_exchange_weak(expected, &second) && expected == &first);
    LFQ_CHECK(pointer.compare_exchange_strong(expected, &second) && pointer.load() == &second);
    pointer.store(nullptr, std::memory_order_release);
    LFQ_CHECK(pointer.load(std::memory_order_acquire) == nullptr);
    uint32_t value = UINT32_MAX;
    LFQ_CHECK(word.compare_exchange_strong(value, 0) && word.load() == 0);
    std::cout << "ATOMIC_VALUE checked_widths=32,64,pointer expected_update=passed" << std::endl;
    return true;
}

bool LockFreeQueueTests::initialBatches()
{
    LockFreeQueue<int32_t> queue;
    LFQ_CHECK(configure(queue, 2049, 2049, 1, 8));
    for (int32_t i = 0; i < 2048; ++i)
    {
        LFQ_CHECK(queue.tryPush(i) == LockFreeQueueSuccess);
    }
    for (int32_t i = 0; i < 2048; ++i)
    {
        int32_t value = -1;
        LFQ_CHECK(queue.pop(&value) && value == i);
    }
    queue.maintain(4096);
    LFQ_CHECK(queue.poolStatistics().usedNodes == 1);
    LockFreeQueueTestControl::reset();
    LockFreeQueueTestControl::m_pausePoint.store(17);
    LockFreeQueueTestThreads threads;
    std::atomic<int32_t> first(-1);
    std::atomic<size_t> published(0);
    threads.m_threads.emplace_back(
        [&]()
        {
            g_lockFreeQueueTestLane = 1;
            // Skip a possible one-bit final word; pause only on a real private batch.
            for (size_t i = 0; i < 32; ++i)
            {
                const LockFreeQueueResult result = queue.tryPush(99999 + static_cast<int32_t>(i));
                first.store(result);
                if (result != LockFreeQueueSuccess)
                {
                    break;
                }
                published.store(i + 1);
                if (LockFreeQueueTestControl::m_release.load())
                {
                    break;
                }
            }
        });
    bool valid = LockFreeQueueTestControl::waitReached();
    const size_t prefix = published.load();
    for (size_t i = 0; i < prefix; ++i)
    {
        int32_t value = -1;
        valid = queue.pop(&value) && value == 99999 + static_cast<int32_t>(i) && valid;
    }
    queue.maintain(4096);
    const LockFreeQueuePoolStatistics paused = queue.poolStatistics();
    const size_t held = paused.usedNodes - 1;
    valid = held > 1 && held <= 32 && paused.reusableFreeCapacity + held == 2048 && valid;
    size_t accepted = 0;
    LockFreeQueueResult stopped = LockFreeQueueSuccess;
    for (; accepted < 2048; ++accepted)
    {
        stopped = queue.tryPush(static_cast<int32_t>(accepted));
        if (stopped != LockFreeQueueSuccess)
        {
            break;
        }
    }
    valid = accepted + held == 2048 && stopped == LockFreeQueueCapacityLimit && valid;
    const LockFreeQueuePoolStatistics full = queue.poolStatistics();
    std::cout << "BATCH_DIAG held=" << held << " accepted=" << accepted << " stopped=" << stopped << " valid=" << valid
              << " full_used=" << full.usedNodes << " full_free=" << full.reusableFreeCapacity
              << " allocations=" << full.allocationCalls << std::endl;
    valid = full.currentCapacity == 2049 && full.peakReservedCapacity == 2049 && full.allocationCalls == 2 &&
            full.reusableFreeCapacity == 0 && valid;
    for (size_t i = 0; i < accepted; ++i)
    {
        int32_t value = -1;
        valid = queue.pop(&value) && value == static_cast<int32_t>(i) && valid;
    }
    LockFreeQueueTestControl::m_release.store(true);
    threads.join();
    int32_t value = -1;
    LFQ_CHECK(valid && first.load() == LockFreeQueueSuccess && queue.pop(&value) &&
              value == 99999 + static_cast<int32_t>(prefix));
    queue.maintain(4096);
    const LockFreeQueuePoolStatistics drained = queue.poolStatistics();
    LFQ_CHECK(queue.empty() && drained.usedNodes == 1 && drained.reusableFreeCapacity == 2048 &&
              drained.retiredNodes == 0 && drained.allocationCalls == 2);
    for (int32_t i = 0; i < 2048; ++i)
    {
        LFQ_CHECK(queue.tryPush(i) == LockFreeQueueSuccess);
    }
    LFQ_CHECK(queue.tryPush(-1) == LockFreeQueueCapacityLimit);
    for (int32_t i = 0; i < 2048; ++i)
    {
        LFQ_CHECK(queue.pop(&value) && value == i);
    }
    std::cout << "INITIAL_BATCH held=" << held << " other_accepted=" << accepted
              << " full_capacity_reused=2048 allocations=" << queue.poolStatistics().allocationCalls << std::endl;
    LockFreeQueueTestControl::reset();
    return true;
}

bool LockFreeQueueTests::growthTransitions()
{
    // No torn quadword is legal: high half must always complement low half.
    LockFreeQueueAtomic<uint64_t> atomic(UINT64_C(0xffffffff00000000));
    std::atomic<uint32_t> finished(0), errors(0), reads(0);
    LockFreeQueueTestThreads atomicThreads;
    for (uint32_t writer = 0; writer < 2; ++writer)
    {
        atomicThreads.m_threads.emplace_back(
            [&, writer]()
            {
                for (uint32_t i = 0; i < 250000; ++i)
                {
                    const uint32_t low = i * UINT32_C(2654435761) + writer;
                    const uint64_t value = (static_cast<uint64_t>(~low) << 32) | low;
                    if ((i & 3) == 0)
                    {
                        atomic.store(value, std::memory_order_release);
                    }
                    else if ((i & 3) == 1)
                    {
                        atomic.exchange(value, std::memory_order_acq_rel);
                    }
                    else
                    {
                        uint64_t expected = atomic.load(std::memory_order_acquire);
                        atomic.compare_exchange_weak(
                            expected, value, std::memory_order_acq_rel, std::memory_order_acquire);
                        if (static_cast<uint32_t>(expected >> 32) != ~static_cast<uint32_t>(expected))
                        {
                            errors.fetch_add(1);
                        }
                    }
                }
                finished.fetch_add(1);
            });
    }
    const uint64_t atomicDeadline = LockFreeQueueTestControl::ticks() + UINT64_C(5000000000);
    while (finished.load() != 2 && LockFreeQueueTestControl::ticks() < atomicDeadline)
    {
        const uint64_t observed = atomic.load(std::memory_order_relaxed);
        if (static_cast<uint32_t>(observed >> 32) != ~static_cast<uint32_t>(observed))
        {
            errors.fetch_add(1);
        }
        reads.fetch_add(1);
    }
    const bool completedInTime = finished.load() == 2;
    atomicThreads.join();
    std::cout << "ATOMIC_CONCURRENT writes=500000 reads=" << reads.load() << " torn=" << errors.load()
              << " completed=" << completedInTime << std::endl;
    LFQ_CHECK(completedInTime && errors.load() == 0 && reads.load() > 0);

    const int32_t points[] = {2, 3, 18, 19};
    for (size_t phase = 0; phase < 4; ++phase)
    {
        LockFreeQueueTestControl::reset();
        LockFreeQueue<int32_t> queue;
        LFQ_CHECK(configure(queue, 4, 8, 0, 4));
        const int32_t prefix = phase < 2 ? 2 : 3;
        for (int32_t i = 0; i < prefix; ++i)
        {
            queue.push(10 + i);
        }
        LFQ_CHECK(queue.poolStatistics().currentCapacity == 4 && queue.poolStatistics().allocationCalls == 2);
        LockFreeQueueTestControl::m_pausePoint.store(points[phase]);
        LockFreeQueueTestThreads threads;
        std::atomic<int32_t> delayed(-1);
        threads.m_threads.emplace_back(
            [&]()
            {
                g_lockFreeQueueTestLane = 1;
                LockFreeQueueResult attempt = queue.tryPush(111);
                if (attempt == LockFreeQueueTemporaryUnavailable)
                {
                    attempt = queue.push(111);
                }
                delayed.store(attempt);
            });
        bool valid = LockFreeQueueTestControl::waitReached();
        const uint64_t begin = LockFreeQueueTestControl::ticks();
        valid = queue.tryPush(222) == LockFreeQueueSuccess && valid;
        for (int32_t i = 0; i < prefix; ++i)
        {
            int32_t value = -1;
            valid = queue.pop(&value) && value == 10 + i && valid;
        }
        int32_t value = -1;
        if (points[phase] == 3)
        {
            valid = queue.pop(&value) && value == 111 && valid;
        }
        valid = queue.pop(&value) && value == 222 && valid;
        // Reuse every physically available node, including initial nodes,
        // while another producer is paused at publication or growth.
        const size_t available = points[phase] == 2 ? 6 : 7;
        for (size_t i = 0; i < available; ++i)
        {
            valid = queue.tryPush(300 + static_cast<int32_t>(i)) == LockFreeQueueSuccess && valid;
        }
        const LockFreeQueueResult full = queue.tryPush(-1);
        valid = full == LockFreeQueueCapacityLimit && valid;
        for (size_t i = 0; i < available; ++i)
        {
            valid = queue.pop(&value) && value == 300 + static_cast<int32_t>(i) && valid;
        }
        const uint64_t independent = LockFreeQueueTestControl::ticks() - begin;
        LockFreeQueueTestControl::m_release.store(true);
        threads.join();
        LFQ_CHECK(valid && delayed.load() == LockFreeQueueSuccess && independent < 1000000000ULL);
        if (points[phase] != 3)
        {
            LFQ_CHECK(queue.pop(&value) && value == 111);
        }
        queue.maintain(2048);
        const LockFreeQueuePoolStatistics result = queue.poolStatistics();
        LFQ_CHECK(queue.empty() && !queue.pop(nullptr) && result.currentCapacity == 8 &&
                  result.peakReservedCapacity == 8 && result.usedNodes == 1 && result.retiredNodes == 0 &&
                  result.allocationCalls == 3 && result.successfulGrowCount == 1);
        std::cout << "GROWTH_SWITCH point=" << points[phase] << " independent_ns=" << independent
                  << " full_nodes_reused=" << available << " capacity=8 allocations=" << queue.poolStatistics().allocationCalls << std::endl;
    }
    for (size_t nearLimit = 0; nearLimit < 2; ++nearLimit)
    {
        LockFreeQueueTestControl::reset();
        LockFreeQueue<int32_t> queue;
        LFQ_CHECK(configure(queue, 3, -1, 0, 2));
        if (nearLimit != 0)
        {
            queue.testSeedGeneration(UINT64_MAX - 15);
        }
        LockFreeQueueTestControl::m_pausePoint.store(nearLimit != 0 ? 2 : 1);
        LockFreeQueueTestThreads threads;
        std::atomic<int32_t> delayed(-1);
        threads.m_threads.emplace_back(
            [&]()
            {
                g_lockFreeQueueTestLane = 1;
                LockFreeQueueResult attempt = queue.tryPush(99999);
                if (attempt == LockFreeQueueTemporaryUnavailable)
                {
                    attempt = queue.push(99999);
                }
                delayed.store(attempt);
            });
        bool valid = LockFreeQueueTestControl::waitReached();
        const int32_t laps = nearLimit != 0 ? 40 : 10000;
        for (int32_t i = 0; i < laps; ++i)
        {
            int32_t value = -1;
            valid = queue.tryPush(i) == LockFreeQueueSuccess && valid;
            valid = queue.pop(&value) && value == i && valid;
        }
        valid = queue.size() == 1 && queue.poolStatistics().currentCapacity == 3 &&
                queue.poolStatistics().allocationCalls == 2 && valid;
        LockFreeQueueTestControl::m_release.store(true);
        threads.join();
        int32_t value = -1;
        LFQ_CHECK(
            valid && delayed.load() == LockFreeQueueSuccess && queue.pop(&value) && value == 99999 && queue.empty());
        std::cout << "GROWTH_REUSE near_limit=" << nearLimit << " laps=" << laps
                  << " stale_hint=passed capacity=3 allocations=" << queue.poolStatistics().allocationCalls << std::endl;
    }
    LockFreeQueueTestControl::reset();
    LockFreeQueueTestControl::m_clock.store(1000);
    LockFreeQueue<int32_t> queue;
    LFQ_CHECK(configure(queue, 4, 64, 0, 4));
    for (int32_t i = 0; i < 3; ++i)
    {
        queue.push(i);
    }
    LockFreeQueueTestControl::m_failPoint.store(11);
    LFQ_CHECK(queue.tryPush(3) == LockFreeQueueAllocationFailure && queue.size() == 3 &&
              queue.poolStatistics().currentCapacity == 4 && queue.poolStatistics().reservedCapacity == 0);
    for (int32_t i = 0; i < 3; ++i)
    {
        int32_t value = -1;
        LFQ_CHECK(queue.pop(&value) && value == i);
    }
    for (int32_t i = 0; i < 31; ++i)
    {
        queue.push(i);
    }
    for (int32_t i = 0; i < 31; ++i)
    {
        int32_t value = -1;
        LFQ_CHECK(queue.pop(&value) && value == i);
    }
    queue.setShrinkPolicy(
        [](const LockFreeQueueShrinkContext&)
        {
            const LockFreeQueueShrinkDecision decision = {64, 10};
            return decision;
        });
    queue.maintain(4096);
    const size_t before = queue.poolStatistics().currentCapacity;
    LockFreeQueueTestControl::m_clock.store(1009);
    queue.maintain(4096);
    LFQ_CHECK(queue.poolStatistics().currentCapacity == before);
    LockFreeQueueTestControl::m_clock.store(1010);
    LockFreeQueueMaintenanceResult detached = {LockFreeQueueSuccess, 0, 0};
    for (size_t step = 0; step < 4096 && queue.poolStatistics().retiringBlockCapacity == 0; ++step)
    {
        detached = queue.maintain(1);
        LFQ_CHECK(detached.result == LockFreeQueueSuccess && detached.releasedCapacity == 0);
    }
    const LockFreeQueuePoolStatistics pending = queue.poolStatistics();
    std::cout << "GROWTH_SHRINK_STAGE capacity=" << pending.currentCapacity
              << " pending=" << pending.retiringBlockCapacity << " released=" << detached.releasedCapacity << std::endl;
    LFQ_CHECK(detached.result == LockFreeQueueSuccess && pending.currentCapacity == before &&
              pending.retiringBlockCapacity > 0 && detached.releasedCapacity == 0);
    // The existing budgeted scheduler closes blocks in its active-block phase;
    // their physical deletion belongs to the next retired-block phase.
    const LockFreeQueueMaintenanceResult shrunk = queue.maintain(4096);
    LFQ_CHECK(shrunk.result == LockFreeQueueSuccess && shrunk.releasedCapacity > 0 &&
              queue.poolStatistics().currentCapacity >= 4 && queue.poolStatistics().currentCapacity < before);
    for (int32_t i = 0; i < 1000; ++i)
    {
        int32_t value = -1;
        LFQ_CHECK(queue.tryPush(i) == LockFreeQueueSuccess && queue.pop(&value) && value == i);
    }
    std::cout << "GROWTH_SHRINK before=" << before << " after=" << queue.poolStatistics().currentCapacity
              << " released=" << shrunk.releasedCapacity << " reuse=1000" << std::endl;
    LockFreeQueueTestControl::reset();
    {
        LockFreeQueue<LockFreeQueueTestValue> values;
        values.destroy();
        LockFreeQueuePoolConfig config;
        config.initialCapacity = 4;
        config.maximumCapacity = 8;
        config.maximumProducers = 2;
        LFQ_CHECK(values.configurePool(config) == LockFreeQueueSuccess && values.tryInit() == LockFreeQueueSuccess);
        LockFreeQueueTestValue source(42), output(0);
        for (size_t i = 0; i < 3; ++i)
        {
            values.push(source);
        }
        LockFreeQueueTestControl::m_clock.store(1000);
        LockFreeQueueTestValue::m_throwCopy = true;
        const LockFreeQueueResult copied = values.tryPush(source);
        LockFreeQueueTestValue::m_throwCopy = false;
        LFQ_CHECK(copied == LockFreeQueueConstructionFailure && values.poolStatistics().currentCapacity == 8);
        for (size_t i = 0; i < 3; ++i)
        {
            LFQ_CHECK(values.pop(&output) && output.m_value == 42);
        }
        size_t callbacks = 0;
        LFQ_CHECK(values.setShrinkPolicy(
                      [&](const LockFreeQueueShrinkContext&)
                      {
                          ++callbacks;
                          const LockFreeQueueShrinkDecision decision = {4, 10};
                          return decision;
                      }) == LockFreeQueueSuccess);
        values.maintain(128);
        LockFreeQueueTestControl::m_clock.store(1010);
        for (size_t step = 0; step < 128 && values.poolStatistics().retiringBlockCapacity == 0; ++step)
        {
            LFQ_CHECK(values.maintain(1).result == LockFreeQueueSuccess);
        }
        const size_t callbacksBeforeRemoval = callbacks;
        LFQ_CHECK(callbacks >= 2 && values.poolStatistics().retiringBlockCapacity == 4 &&
                  values.poolStatistics().currentCapacity == 8);
        LFQ_CHECK(values.setShrinkPolicy(LockFreeQueueShrinkPolicy()) == LockFreeQueueSuccess);
        for (size_t i = 0; i < 64; ++i)
        {
            values.push(source);
            LFQ_CHECK(values.pop(&output) && output.m_value == 42);
        }
        const LockFreeQueuePoolStatistics returned = values.poolStatistics();
        LFQ_CHECK(callbacks == callbacksBeforeRemoval && returned.currentCapacity == 4 &&
                  returned.retiringBlockCapacity == 0 && returned.usedNodes == 1 && returned.successfulGrowCount == 1);
        std::cout << "GROWTH_PENDING_RELEASE after_policy_removed=4 initial_capacity=4" << std::endl;
    }
    LFQ_CHECK(configure(queue, 65, -1, 0, 2));
    size_t callbacks = 0;
    LFQ_CHECK(queue.setShrinkPolicy(
                  [&](const LockFreeQueueShrinkContext&)
                  {
                      ++callbacks;
                      const LockFreeQueueShrinkDecision decision = {0, 0};
                      return decision;
                  }) == LockFreeQueueSuccess);
    for (int32_t i = 0; i < 128; ++i)
    {
        if (i == 64)
        {
            LFQ_CHECK(callbacks == 1 && queue.setShrinkPolicy(LockFreeQueueShrinkPolicy()) == LockFreeQueueSuccess);
        }
        int32_t value = -1;
        queue.push(i);
        LFQ_CHECK(queue.pop(&value) && value == i);
    }
    LFQ_CHECK(callbacks == 1 && queue.poolStatistics().successfulGrowCount == 0);
    std::cout << "GROWTH_POLICY enabled_callbacks=1 disabled_callbacks=0" << std::endl;
    LockFreeQueueTestControl::reset();
    return queue.empty();
}

bool LockFreeQueueTests::fifo()
{
    LockFreeQueue<int32_t> queue;
    int32_t value = -1;
    LFQ_CHECK(queue.atomicsLockFree());
    LFQ_CHECK(!queue.pop(&value) && value == -1);
    for (int32_t i = 0; i < 10000; ++i)
    {
        queue.push(i);
    }
    LFQ_CHECK(queue.size() == 10000);
    for (int32_t i = 0; i < 10000; ++i)
    {
        LFQ_CHECK(queue.pop(&value) && value == i);
    }
    for (int32_t i = 0; i < 1000; ++i)
    {
        queue.push(i);
        LFQ_CHECK(queue.pop(nullptr) && !queue.pop(nullptr));
    }
    LFQ_CHECK(queue.empty() && queue.poolStatistics().usedNodes <= 32);
    LFQ_CHECK(queue.maintain(256).result == LockFreeQueueSuccess);
    LFQ_CHECK(queue.poolStatistics().usedNodes == 1);
    return true;
}

bool LockFreeQueueTests::lifecycle()
{
    for (size_t i = 0; i < 100; ++i)
    {
        LockFreeQueue<int32_t> a;
        LockFreeQueue<int32_t> b;
        a.push(1);
        b.push(2);
        a.init();
        int32_t value = 0;
        LFQ_CHECK(a.empty() && b.pop(&value) && value == 2);
        a.destroy();
        a.destroy();
        LFQ_CHECK(a.tryPush(3) == LockFreeQueueNotInitialized);
        LFQ_CHECK(a.tryInit() == LockFreeQueueSuccess);
        a.push(4);
        LFQ_CHECK(a.pop(&value) && value == 4);
    }
    return true;
}

bool LockFreeQueueTests::ownership()
{
    LockFreeQueue<std::shared_ptr<LockFreeQueueTestValue>> queue;
    std::shared_ptr<LockFreeQueueTestValue> value(new LockFreeQueueTestValue(42));
    std::weak_ptr<LockFreeQueueTestValue> weak(value);
    queue.push(value);
    value.reset();
    LFQ_CHECK(!weak.expired());
    LFQ_CHECK(queue.pop(nullptr) && weak.expired());
    LockFreeQueue<LockFreeQueueTestValue*> raw;
    std::unique_ptr<LockFreeQueueTestValue> owned(new LockFreeQueueTestValue(7));
    raw.push(owned.get());
    raw.clear();
    LFQ_CHECK(owned->m_value == 7);
    owned.reset();
    LFQ_CHECK(LockFreeQueueTestValue::m_live.load() == 0);
    return true;
}

bool LockFreeQueueTests::reuse()
{
    LockFreeQueue<int32_t> queue;
    LFQ_CHECK(configure(queue, 8, 8));
    const uint64_t allocations = queue.poolStatistics().allocationCalls;
    for (int32_t round = 0; round < 10000; ++round)
    {
        for (int32_t i = 0; i < 7; ++i)
        {
            LFQ_CHECK(queue.tryPush(i) == LockFreeQueueSuccess);
        }
        queue.clear();
    }
    LFQ_CHECK(queue.poolStatistics().allocationCalls == allocations);
    LFQ_CHECK(queue.poolStatistics().usedNodes == 1 && queue.poolStatistics().retiredNodes == 0);
    // Fill across multiple growth blocks twice. Reusing known-free storage must
    // not repeatedly inspect every occupied node in an earlier full block.
    for (size_t threshold = 0; threshold <= 1; ++threshold)
    {
        LockFreeQueue<int32_t> grown;
        LFQ_CHECK(configure(grown, 64, 4096, threshold));
        for (int32_t i = 0; i < 4095; ++i)
        {
            LFQ_CHECK(grown.tryPush(i) == LockFreeQueueSuccess);
        }
        for (int32_t i = 0; i < 4095; ++i)
        {
            int32_t value = -1;
            LFQ_CHECK(grown.pop(&value) && value == i);
        }
        for (size_t pass = 0; pass < 8; ++pass)
        {
            LFQ_CHECK(grown.maintain(32768).result == LockFreeQueueSuccess);
        }
        const LockFreeQueuePoolStatistics before = grown.poolStatistics();
        LFQ_CHECK(before.currentCapacity == 4096 && before.usedNodes == 1 && before.retiredNodes == 0);
        LockFreeQueueTestControl::m_reclaimVisits.store(0);
        for (int32_t i = 0; i < 4095; ++i)
        {
            LFQ_CHECK(grown.tryPush(10000 + i) == LockFreeQueueSuccess);
        }
        const size_t visits = LockFreeQueueTestControl::m_reclaimVisits.load();
        std::cout << "GROWN_REUSE threshold=" << threshold << " reclaim_visits=" << visits
                  << " payloads=4095 capacity=4096" << std::endl;
        // Positive threshold may request two complete reclamation passes near
        // the configured limit; this is linear in capacity, not per allocation.
        LFQ_CHECK(visits <= 4096 * 4);
        LFQ_CHECK(grown.tryPush(99999) == LockFreeQueueCapacityLimit);
        const LockFreeQueuePoolStatistics filled = grown.poolStatistics();
        LFQ_CHECK(filled.allocationCalls == before.allocationCalls &&
                  filled.successfulGrowCount == before.successfulGrowCount && filled.peakReservedCapacity == 4096);
        for (int32_t i = 0; i < 4095; ++i)
        {
            int32_t value = -1;
            LFQ_CHECK(grown.pop(&value) && value == 10000 + i);
        }
        LFQ_CHECK(grown.empty());
    }
    return true;
}

LockFreeQueueTestMove::LockFreeQueueTestMove(int32_t value) : m_value(value)
{
}

LockFreeQueueTestMove::LockFreeQueueTestMove(const LockFreeQueueTestMove& other) : m_value(other.m_value)
{
}

LockFreeQueueTestMove& LockFreeQueueTestMove::operator=(const LockFreeQueueTestMove& other)
{
    (void)other;
    throw std::runtime_error("unexpected copy assignment");
}

LockFreeQueueTestMove& LockFreeQueueTestMove::operator=(LockFreeQueueTestMove&& other) LOCK_FREE_QUEUE_TEST_NOTHROW
{
    m_value = other.m_value;
    other.m_value = -777;
    return *this;
}

LockFreeQueueTestThrowMove::LockFreeQueueTestThrowMove(int32_t value) : m_value(value)
{
}

LockFreeQueueTestThrowMove::LockFreeQueueTestThrowMove(const LockFreeQueueTestThrowMove& other) : m_value(other.m_value)
{
}

LockFreeQueueTestThrowMove& LockFreeQueueTestThrowMove::operator=(const LockFreeQueueTestThrowMove& other)
{
    m_value = other.m_value;
    return *this;
}

LockFreeQueueTestThrowMove& LockFreeQueueTestThrowMove::operator=(LockFreeQueueTestThrowMove&& other)
{
    other.m_value = -888;
    throw std::runtime_error("unsafe throwing move selected");
}

bool LockFreeQueueTests::exceptions()
{
    static_assert(std::is_nothrow_move_assignable<LockFreeQueueTestMove>::value, "nothrow move detection");
    static_assert(!std::is_nothrow_move_assignable<LockFreeQueueTestThrowMove>::value, "throwing move detection");
    for (int32_t mode = 0; mode < 2; ++mode)
    {
        LockFreeQueuePoolConfig config;
        config.initialCapacity = 8;
        config.maximumCapacity = mode == 0 ? 8 : -1;
        LockFreeQueue<LockFreeQueueTestMove> movable;
        LockFreeQueue<LockFreeQueueTestThrowMove> fallback;
        movable.destroy();
        fallback.destroy();
        LFQ_CHECK(movable.configurePool(config) == LockFreeQueueSuccess && movable.tryInit() == LockFreeQueueSuccess);
        LFQ_CHECK(fallback.configurePool(config) == LockFreeQueueSuccess && fallback.tryInit() == LockFreeQueueSuccess);
        LockFreeQueueTestMove input(41), output(0);
        LockFreeQueueTestThrowMove copyInput(42), copyOutput(0);
        movable.push(input);
        fallback.push(copyInput);
        LFQ_CHECK(input.m_value == 41 && copyInput.m_value == 42);
        LFQ_CHECK(movable.pop(&output) && output.m_value == 41 && movable.empty());
        LFQ_CHECK(fallback.pop(&copyOutput) && copyOutput.m_value == 42 && fallback.empty());
    }

    {
        LockFreeQueue<LockFreeQueueTestValue> queue;
        LockFreeQueueTestValue source(42);
        LockFreeQueueTestValue output(0);
        LockFreeQueueTestValue::m_throwCopy = true;
        LFQ_CHECK(queue.tryPush(source) == LockFreeQueueConstructionFailure);
        bool caught = false;
        const LockFreeQueueResult pushed = queue.push(source);
        LockFreeQueueTestValue::m_throwCopy = false;
        LFQ_CHECK(pushed == LockFreeQueueConstructionFailure && queue.empty() && queue.poolStatistics().usedNodes == 1);
        queue.push(source);
        LockFreeQueueTestValue::m_throwAssign = true;
        caught = false;
        try
        {
            queue.pop(&output);
        }
        catch (const std::runtime_error&)
        {
            caught = true;
        }
        LockFreeQueueTestValue::m_throwAssign = false;
        LFQ_CHECK(caught && output.m_value == -999 && queue.size() == 1);
        LFQ_CHECK(queue.pop(&output) && output.m_value == 42);
    }
    LFQ_CHECK(LockFreeQueueTestValue::m_live.load() == 0);
    return true;
}

bool LockFreeQueueTests::boundaries()
{
    LockFreeQueue<int32_t> deferred(false);
    LFQ_CHECK(deferred.poolStatistics().currentCapacity == 0 && deferred.poolStatistics().allocationCalls == 0);
    LFQ_CHECK(deferred.tryPush(3) == LockFreeQueueNotInitialized && !deferred.pop(nullptr));
    LockFreeQueuePoolConfig directConfig;
    directConfig.initialCapacity = 4;
    directConfig.maximumCapacity = 4;
    directConfig.maximumProducers = 2;
    LFQ_CHECK(deferred.configurePool(directConfig) == LockFreeQueueSuccess);
    LFQ_CHECK(deferred.tryInit() == LockFreeQueueSuccess && deferred.poolStatistics().currentCapacity == 4);
    LFQ_CHECK(deferred.tryPush(17) == LockFreeQueueSuccess);
    int32_t directValue = 0;
    LFQ_CHECK(deferred.pop(&directValue) && directValue == 17);
    deferred.destroy();
    LFQ_CHECK(deferred.tryInit() == LockFreeQueueSuccess && deferred.tryPush(19) == LockFreeQueueSuccess);
    LFQ_CHECK(deferred.pop(&directValue) && directValue == 19);
    LockFreeQueue<int32_t> immediate(true);
    LFQ_CHECK(immediate.tryPush(23) == LockFreeQueueSuccess && immediate.pop(&directValue) && directValue == 23);
    LockFreeQueue<int32_t> queue;
    queue.destroy();
    LockFreeQueuePoolConfig config;
    config.maximumCapacity = -2;
    LFQ_CHECK(queue.configurePool(config) == LockFreeQueueInvalidConfiguration);
    config.maximumCapacity = 63;
    LFQ_CHECK(queue.configurePool(config) == LockFreeQueueInvalidConfiguration);
    config.initialCapacity = 0;
    LFQ_CHECK(queue.configurePool(config) == LockFreeQueueInvalidConfiguration);
    config.initialCapacity = 1;
    config.maximumCapacity = 1;
    config.maximumProducers = 0;
    LFQ_CHECK(queue.configurePool(config) == LockFreeQueueInvalidConfiguration);
    config.maximumProducers = (std::numeric_limits<size_t>::max)();
    LFQ_CHECK(queue.configurePool(config) == LockFreeQueueInvalidConfiguration);
    config.maximumProducers = 2;
    LFQ_CHECK(queue.configurePool(config) == LockFreeQueueSuccess);
    LFQ_CHECK(queue.tryPush(1) == LockFreeQueueNotInitialized);
    LFQ_CHECK(queue.tryInit() == LockFreeQueueSuccess);
    LFQ_CHECK(queue.tryPush(1) == LockFreeQueueCapacityLimit);
    LFQ_CHECK(queue.push(1) == LockFreeQueueCapacityLimit && queue.empty());
    LFQ_CHECK(queue.configurePool(config) == LockFreeQueueInvalidState);
    return true;
}

bool LockFreeQueueTests::thresholds()
{
    LockFreeQueue<int32_t> queue;
    LFQ_CHECK(configure(queue, 3, 5));
    queue.setGrowPolicy([](const LockFreeQueueGrowContext&) { return size_t(2); });
    queue.push(1);
    queue.push(2);
    LFQ_CHECK(queue.poolStatistics().currentCapacity == 3 && queue.poolStatistics().successfulGrowCount == 0);
    LFQ_CHECK(queue.tryPush(3) == LockFreeQueueSuccess);
    LFQ_CHECK(queue.poolStatistics().currentCapacity == 5 && queue.poolStatistics().successfulGrowCount == 1);
    queue.push(4);
    LFQ_CHECK(queue.tryPush(5) == LockFreeQueueCapacityLimit);
    LFQ_CHECK(queue.pop(nullptr));
    LFQ_CHECK(queue.tryPush(5) == LockFreeQueueSuccess);
    LFQ_CHECK(configure(queue, 3, 5, 1));
    queue.push(1);
    LFQ_CHECK(queue.poolStatistics().currentCapacity == 5);
    LFQ_CHECK(configure(queue, 3, 5, 1));
    LockFreeQueueTestControl::m_failPoint.store(11);
    LFQ_CHECK(queue.tryPush(1) == LockFreeQueueSuccess);
    LFQ_CHECK(queue.size() == 1 && queue.poolStatistics().currentCapacity == 3);
    LFQ_CHECK(queue.poolStatistics().reservedCapacity == 0);
    return true;
}

bool LockFreeQueueTests::policies()
{
    LockFreeQueue<int32_t> queue;
    LFQ_CHECK(configure(queue, 2, 5));
    queue.setGrowPolicy([](const LockFreeQueueGrowContext&) { return size_t(0); });
    queue.push(1);
    LFQ_CHECK(queue.tryPush(2) == LockFreeQueueGrowthDeclined);
    queue.setGrowPolicy([](const LockFreeQueueGrowContext&) -> size_t { throw std::runtime_error("policy"); });
    LFQ_CHECK(queue.tryPush(2) == LockFreeQueuePolicyFailure);
    queue.setGrowPolicy(
        [](const LockFreeQueueGrowContext& context)
        {
            if (context.initialCapacity != 2 || context.currentCapacity != 2 || context.successfulGrowCount != 0)
            {
                return size_t(0);
            }
            return (std::numeric_limits<size_t>::max)();
        });
    LFQ_CHECK(queue.tryPush(2) == LockFreeQueueSuccess);
    LFQ_CHECK(queue.poolStatistics().currentCapacity == 5 && queue.poolStatistics().successfulGrowCount == 1);
    LockFreeQueueGrowPolicy policy = LockFreeQueueTestPolicy();
    LockFreeQueueTestPolicy::m_throwCopy = true;
    const LockFreeQueueResult copied = queue.setGrowPolicy(policy);
    LockFreeQueueTestPolicy::m_throwCopy = false;
    LFQ_CHECK(copied == LockFreeQueueAllocationFailure);
    std::atomic<uint64_t> saturated((std::numeric_limits<uint64_t>::max)() - 1);
    LockFreeQueueArena<int32_t>::testIncrement(saturated);
    LockFreeQueueArena<int32_t>::testIncrement(saturated);
    LFQ_CHECK(saturated.load() == (std::numeric_limits<uint64_t>::max)());
    LFQ_CHECK(configure(queue, 2, -1));
    queue.setGrowPolicy(LockFreeQueueGrowPolicy());
    queue.push(1);
    queue.push(2);
    LFQ_CHECK(queue.poolStatistics().currentCapacity == 4);
    LFQ_CHECK(configure(queue, 3, 10));
    queue.setGrowPolicy(
        [](const LockFreeQueueGrowContext& context)
        {
            return context.currentCapacity / 2; // Deliberate integer rounding: 3 / 2 == 1.
        });
    queue.push(1);
    queue.push(2);
    queue.push(3);
    LFQ_CHECK(queue.poolStatistics().currentCapacity == 4);
    queue.push(4);
    LFQ_CHECK(queue.poolStatistics().currentCapacity == 6 && queue.poolStatistics().successfulGrowCount == 2);
    queue.setShrinkPolicy([](const LockFreeQueueShrinkContext&) -> LockFreeQueueShrinkDecision
        { throw std::runtime_error("shrink policy"); });
    LFQ_CHECK(queue.maintain(32).result == LockFreeQueuePolicyFailure);
    LockFreeQueueTestControl::m_clock.store(0);
    LFQ_CHECK(queue.maintain(32).result == LockFreeQueueClockUnavailable);
    LFQ_CHECK(queue.maintain(0).result == LockFreeQueueSuccess);
    LockFreeQueueTestControl::m_clock.store(UINT64_MAX);
    return true;
}

bool LockFreeQueueTests::allocations()
{
    {
        LockFreeQueueArena<int32_t> pool;
        LockFreeQueuePoolConfig config;
        config.initialCapacity = 1;
        config.maximumCapacity = 1;
        config.maximumProducers = 1;
        LFQ_CHECK(pool.configure(config) == LockFreeQueueSuccess && pool.initialize() == LockFreeQueueSuccess);
        auto record = pool.enter();
        std::vector<LockFreeQueueArena<int32_t>::Segment*> segments;
        for (size_t i = 0; i < 3; ++i)
        {
            auto segment = pool.acquireSegment(*record);
            LFQ_CHECK(segment != nullptr);
            segments.push_back(segment);
        }
        LockFreeQueueTestControl::m_failPoint.store(12);
        LockFreeQueueResult failed;
        LFQ_CHECK(pool.acquireSegment(*record, false, &failed) == nullptr && failed == LockFreeQueueAllocationFailure);
        LFQ_CHECK(pool.statistics().currentCapacity == 1 && pool.statistics().reservedCapacity == 0);
        auto recovered = pool.acquireSegment(*record, false, &failed);
        LFQ_CHECK(recovered != nullptr && failed == LockFreeQueueSuccess);
        pool.releaseSegment(recovered, record);
        for (size_t i = 0; i < segments.size(); ++i)
        {
            pool.releaseSegment(segments[i], record);
        }
        pool.leave(*record, true);
    }
    for (int32_t point = 10; point <= 11; ++point)
    {
        LockFreeQueue<int32_t> queue;
        queue.destroy();
        LockFreeQueueTestControl::m_failPoint.store(point);
        LFQ_CHECK(queue.tryInit() == LockFreeQueueAllocationFailure);
        LFQ_CHECK(queue.tryPush(1) == LockFreeQueueNotInitialized);
        LFQ_CHECK(queue.poolStatistics().currentCapacity == 0 && queue.poolStatistics().reservedCapacity == 0);
        LFQ_CHECK(queue.tryInit() == LockFreeQueueSuccess);
        LFQ_CHECK(configure(queue, 2, 8));
        queue.push(1);
        if (point != 10)
        {
            LockFreeQueueTestControl::m_failPoint.store(point);
            LFQ_CHECK(queue.tryPush(2) == LockFreeQueueAllocationFailure);
            LFQ_CHECK(queue.size() == 1 && queue.poolStatistics().reservedCapacity == 0);
            LFQ_CHECK(queue.poolStatistics().successfulGrowCount == 0);
            LFQ_CHECK(queue.tryPush(2) == LockFreeQueueSuccess);
        }
    }
    return true;
}

bool LockFreeQueueTests::mpsc(size_t producers, size_t count, bool bounded)
{
    LockFreeQueue<uint64_t> queue;
    queue.destroy();
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 64;
    config.maximumCapacity = bounded ? 256 : -1;
    config.maximumProducers = producers + 4;
    LFQ_CHECK(queue.configurePool(config) == LockFreeQueueSuccess && queue.tryInit() == LockFreeQueueSuccess);
    const size_t total = producers * count;
    std::vector<uint8_t> accepted(total, 0);
    std::vector<uint8_t> consumed(total, 0);
    std::vector<size_t> next(producers, 0);
    std::atomic<bool> start(false);
    std::atomic<bool> stop(false);
    std::atomic<size_t> done(0);
    std::atomic<size_t> rejected(0);
    std::atomic<bool> threadError(false);
    LockFreeQueueTestThreads threads;
    try
    {
        for (size_t lane = 0; lane < producers; ++lane)
        {
            threads.m_threads.emplace_back(
                [&, lane]()
                {
                    try
                    {
                        while (!start.load() && !stop.load())
                        {
                            std::this_thread::yield();
                        }
                        for (size_t seq = 0; seq < count && !stop.load(); ++seq)
                        {
                            const uint64_t id = lane * count + seq;
                            for (;;)
                            {
                                const LockFreeQueueResult result = queue.tryPush(id);
                                if (result == LockFreeQueueSuccess)
                                {
                                    accepted[static_cast<size_t>(id)] = 1;
                                    break;
                                }
                                if ((result != LockFreeQueueCapacityLimit &&
                                        !LockFreeQueueBenchmarkPush::transient(result)) ||
                                    stop.load())
                                {
                                    threadError.store(true);
                                    break;
                                }
                                ++rejected;
                                std::this_thread::yield();
                            }
                        }
                    }
                    catch (...)
                    {
                        threadError.store(true);
                    }
                    ++done;
                });
        }
    }
    catch (...)
    {
        stop.store(true);
        start.store(true);
        throw;
    }
    const uint64_t started = LockFreeQueueTestControl::ticks();
    const uint64_t deadline = started + 30000000000ULL;
    start.store(true);
    size_t received = 0;
    bool correct = true;
    while ((done.load() != producers || !queue.empty()) && LockFreeQueueTestControl::ticks() < deadline)
    {
        uint64_t id = 0;
        if (!queue.pop(&id))
        {
            std::this_thread::yield();
            continue;
        }
        ++received;
        if (id >= total || consumed[static_cast<size_t>(id)] != 0)
        {
            correct = false;
            break;
        }
        consumed[static_cast<size_t>(id)] = 1;
        const size_t lane = static_cast<size_t>(id) / count;
        const size_t seq = static_cast<size_t>(id) % count;
        if (seq != next[lane]++)
        {
            correct = false;
            break;
        }
    }
    stop.store(true);
    threads.join();
    const LockFreeQueuePoolStatistics stats = queue.poolStatistics();
    std::cout << "MPSC producers=" << producers << " per_producer=" << count << " received=" << received
              << " rejected_retries=" << rejected.load() << " peak_capacity=" << stats.peakReservedCapacity
              << " allocations=" << stats.allocationCalls << " cas_retries=" << stats.casRetries
              << " elapsed_ns=" << LockFreeQueueTestControl::ticks() - started << std::endl;
    LFQ_CHECK(correct && !threadError.load() && received == total && accepted == consumed && queue.empty());
    LFQ_CHECK(!bounded || stats.peakReservedCapacity <= 256);
    queue.maintain(4096);
    LFQ_CHECK(queue.poolStatistics().usedNodes == 1 && queue.poolStatistics().retiredNodes == 0);
    return true;
}

bool LockFreeQueueTests::concurrentClear()
{
    {
        LockFreeQueue<LockFreeQueueTestValue> queue;
        std::atomic<size_t> finished(0);
        std::atomic<bool> stop(false);
        std::atomic<bool> failed(false);
        LockFreeQueueTestThreads threads;
        try
        {
            for (size_t lane = 0; lane < 4; ++lane)
            {
                threads.m_threads.emplace_back(
                    [&]()
                    {
                        try
                        {
                            LockFreeQueueTestValue value(123);
                            for (size_t i = 0; i < 5000 && !stop.load(); ++i)
                            {
                                for (;;)
                                {
                                    const LockFreeQueueResult result = queue.tryPush(value);
                                    if (result == LockFreeQueueSuccess)
                                    {
                                        break;
                                    }
                                    if (!LockFreeQueueBenchmarkPush::transient(result) || stop.load())
                                    {
                                        failed.store(true);
                                        break;
                                    }
                                    std::this_thread::yield();
                                }
                            }
                        }
                        catch (...)
                        {
                            failed.store(true);
                        }
                        ++finished;
                    });
            }
        }
        catch (...)
        {
            stop.store(true);
            throw;
        }
        // Finite producers guarantee clear eventually sees an empty link.
        const uint64_t deadline = LockFreeQueueTestControl::ticks() + 10000000000ULL;
        while (finished.load() != 4 && LockFreeQueueTestControl::ticks() < deadline)
        {
            queue.clear();
            std::this_thread::yield();
        }
        stop.store(true);
        threads.join();
        queue.clear();
        LFQ_CHECK(!failed.load() && queue.empty());
    }
    LFQ_CHECK(LockFreeQueueTestValue::m_live.load() == 0);
    return true;
}

bool LockFreeQueueTests::publication(int32_t point)
{
    LockFreeQueue<int32_t> queue;
    LFQ_CHECK(configure(queue, 8, 8));
    LockFreeQueueTestControl::m_pausePoint.store(point);
    std::atomic<int32_t> result(-1);
    LockFreeQueueTestThreads threads;
    threads.m_threads.emplace_back(
        [&]()
        {
            g_lockFreeQueueTestLane = 1;
            const LockFreeQueueResult attempted = queue.tryPush(111);
            result.store(attempted == LockFreeQueueTemporaryUnavailable ? queue.push(111) : attempted);
        });
    bool correct = LockFreeQueueTestControl::waitReached();
    const uint64_t begin = LockFreeQueueTestControl::ticks();
    correct = queue.tryPush(222) == LockFreeQueueSuccess && correct;
    int32_t value = 0;
    correct = queue.pop(&value) && value == (point == 3 ? 111 : 222) && correct;
    if (point == 3)
    {
        correct = queue.pop(&value) && value == 222 && correct;
    }
    correct = !queue.pop(nullptr) && correct;
    const uint64_t elapsed = LockFreeQueueTestControl::ticks() - begin;
    LockFreeQueueTestControl::m_release.store(true);
    threads.join();
    LFQ_CHECK(correct && result.load() == LockFreeQueueSuccess && elapsed < 1000000000ULL);
    if (point != 3)
    {
        LFQ_CHECK(queue.pop(&value) && value == 111);
    }
    LFQ_CHECK(queue.empty());
    std::cout << "PAUSE point=" << point << " independent_progress_ns=" << elapsed << std::endl;
    return true;
}

bool LockFreeQueueTests::publicationOrder()
{
    const int32_t points[] = {2, 3};
    for (size_t i = 0; i < 2; ++i)
    {
        LockFreeQueueTestControl::reset();
        LockFreeQueue<int32_t> queue;
        LFQ_CHECK(configure(queue, 8, 8));
        LockFreeQueueTestControl::m_pausePoint.store(points[i]);
        std::atomic<int32_t> firstResult(-1);
        LockFreeQueueTestThreads threads;
        threads.m_threads.emplace_back(
            [&]()
            {
                g_lockFreeQueueTestLane = 1;
                const LockFreeQueueResult attempted = queue.tryPush(111);
                firstResult.store(attempted == LockFreeQueueTemporaryUnavailable ? queue.push(111) : attempted);
            });
        const bool reached = LockFreeQueueTestControl::waitReached();
        const LockFreeQueueResult secondResult = queue.tryPush(222);
        // No pop until BOTH producers have returned. A private preparation
        // ticket must not move an unpublished item ahead of a completed publish.
        LockFreeQueueTestControl::m_release.store(true);
        threads.join();
        int32_t first = -1;
        int32_t second = -1;
        const bool poppedFirst = queue.pop(&first);
        const bool poppedSecond = queue.pop(&second);
        const int32_t expectedFirst = points[i] == 2 ? 222 : 111;
        const int32_t expectedSecond = points[i] == 2 ? 111 : 222;
        std::cout << "PUBLICATION_ORDER point=" << points[i] << " first=" << first << " second=" << second
                  << " expected_first=" << expectedFirst << " expected_second=" << expectedSecond << std::endl;
        LFQ_CHECK(reached && !LockFreeQueueTestControl::m_timedOut.load() &&
                  firstResult.load() == LockFreeQueueSuccess && secondResult == LockFreeQueueSuccess && poppedFirst &&
                  poppedSecond && first == expectedFirst && second == expectedSecond && !queue.pop(nullptr) &&
                  queue.empty());
    }
    return true;
}

bool LockFreeQueueTests::reservations()
{
    for (size_t phase = 0; phase < 2; ++phase)
    {
        LockFreeQueue<int32_t> queue;
        LFQ_CHECK(configure(queue, 1, 9));
        queue.setGrowPolicy([](const LockFreeQueueGrowContext&) { return size_t(4); });
        LockFreeQueueTestControl::reset();
        LockFreeQueueTestControl::m_pausePoint.store(phase == 0 ? 11 : 8);
        std::atomic<int32_t> result(-1);
        LockFreeQueueTestThreads threads;
        threads.m_threads.emplace_back(
            [&]()
            {
                g_lockFreeQueueTestLane = 1;
                result.store(queue.tryPush(111));
            });
        bool correct = LockFreeQueueTestControl::waitReached();
        const LockFreeQueuePoolStatistics stats = queue.poolStatistics();
        correct = stats.peakReservedCapacity <= 9 && correct;
        const uint64_t started = LockFreeQueueTestControl::ticks();
        const LockFreeQueueResult second = queue.tryPush(222);
        const uint64_t elapsed = LockFreeQueueTestControl::ticks() - started;
        correct = second == LockFreeQueueSuccess && correct;
        if (second == LockFreeQueueSuccess)
        {
            int32_t value = 0;
            correct = queue.pop(&value) && value == 222 && correct;
        }
        LockFreeQueueTestControl::m_release.store(true);
        threads.join();
        LFQ_CHECK(correct && elapsed < 1000000000ULL && result.load() == LockFreeQueueSuccess);
        LFQ_CHECK(queue.poolStatistics().reservedCapacity == 0 && queue.poolStatistics().currentCapacity == 5);
    }
    return mpsc(8, 2000, true);
}

bool LockFreeQueueTests::shrink()
{
    LockFreeQueue<int32_t> queue;
    LFQ_CHECK(configure(queue, 2, 16));
    queue.setGrowPolicy([](const LockFreeQueueGrowContext&) { return size_t(2); });
    for (int32_t i = 0; i < 7; ++i)
    {
        queue.push(i);
    }
    LFQ_CHECK(queue.poolStatistics().currentCapacity == 8);
    queue.clear();
    queue.setShrinkPolicy(
        [](const LockFreeQueueShrinkContext&)
        {
            LockFreeQueueShrinkDecision decision = {1000, 0};
            return decision;
        });
    queue.maintain(256);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    queue.maintain(256);
    LFQ_CHECK(queue.poolStatistics().currentCapacity == 8);
    uint64_t idle = 40;
    bool validContext = true;
    queue.setShrinkPolicy(
        [&](const LockFreeQueueShrinkContext& context)
        {
            validContext = validContext && context.initialCapacity == 2 && context.successfulGrowCount == 3 &&
                           context.hasLastSuccessfulGrow && context.currentCapacity >= 2;
            LockFreeQueueShrinkDecision decision = {1000, idle};
            return decision;
        });
    queue.maintain(256);
    LFQ_CHECK(queue.poolStatistics().currentCapacity == 8);
    idle = 5;
    queue.maintain(256);
    queue.maintain(256);
    // The empty publication segment returns to the initial block; all grown payload blocks can be released.
    LFQ_CHECK(validContext && queue.poolStatistics().currentCapacity == 2);
    LFQ_CHECK(queue.poolStatistics().successfulGrowCount == 3 && queue.poolStatistics().retiringBlockCapacity == 0);
    queue.setShrinkPolicy(LockFreeQueueShrinkPolicy());
    for (int32_t i = 0; i < 7; ++i)
    {
        queue.push(i);
    }
    LFQ_CHECK(queue.poolStatistics().successfulGrowCount > 3 && queue.poolStatistics().peakReservedCapacity <= 16);
    queue.clear();
    return true;
}

bool LockFreeQueueTests::poolInterleavings()
{
    // The preceding transfer test also uses the shared pause flags.
    // Reset them before arranging this independent protected-block pause.
    LockFreeQueueTestControl::reset();
    // Pause a producer while it protects a pool block, with no acquired slot yet.
    LockFreeQueue<int32_t> queue;
    LFQ_CHECK(configure(queue, 2, 16));
    queue.setGrowPolicy([](const LockFreeQueueGrowContext&) { return size_t(2); });
    for (int32_t i = 0; i < 7; ++i)
    {
        queue.push(i);
    }
    queue.clear();
    LockFreeQueueTestControl::m_clock.store(1000);
    queue.setShrinkPolicy(
        [](const LockFreeQueueShrinkContext&)
        {
            LockFreeQueueShrinkDecision decision = {1000, 1};
            return decision;
        });
    queue.maintain(256); // Drain free-node caches before testing block traversal.
    LockFreeQueueTestControl::m_pausePoint.store(6);
    LockFreeQueueTestThreads threads;
    std::atomic<int32_t> pushed(-1);
    threads.m_threads.emplace_back(
        [&]()
        {
            g_lockFreeQueueTestLane = 1;
            pushed.store(queue.tryPush(99));
        });
    bool correct = LockFreeQueueTestControl::waitReached();
    LockFreeQueueTestControl::m_clock.store(1005);
    queue.maintain(256);
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    queue.maintain(256);
    queue.maintain(256);
    // The protected head block and initial block must remain; other idle blocks can leave.
    correct = queue.poolStatistics().currentCapacity == 4 && correct;
    LockFreeQueueTestControl::m_release.store(true);
    threads.join();
    std::cout << "POOL_PROTECTION reached=" << LockFreeQueueTestControl::m_reached.load() << " capacity=" << queue.poolStatistics().currentCapacity << " pending=" << queue.poolStatistics().retiringBlockCapacity << " pushed=" << pushed.load() << " clock=" << LockFreeQueueTestControl::m_clock.load() << " correct=" << correct << std::endl;
    LFQ_CHECK(correct && pushed.load() == LockFreeQueueSuccess);
    int32_t value = 0;
    LFQ_CHECK(queue.pop(&value) && value == 99);
    LockFreeQueueTestControl::m_clock.store(UINT64_MAX);
    // Sustain consumer-side shrinking concurrently with allocation and safe slot reuse.
    std::atomic<bool> done(false);
    std::atomic<bool> failed(false);
    std::atomic<bool> stop(false);
    threads.m_threads.emplace_back(
        [&]()
        {
            for (int32_t i = 0; i < 5000 && !stop.load(); ++i)
            {
                for (;;)
                {
                    const LockFreeQueueResult result = queue.tryPush(i);
                    if (result == LockFreeQueueSuccess)
                    {
                        break;
                    }
                    if (stop.load() ||
                        (!LockFreeQueueBenchmarkPush::transient(result) && result != LockFreeQueueCapacityLimit))
                    {
                        failed.store(true);
                        break;
                    }
                    std::this_thread::yield();
                }
            }
            done.store(true);
        });
    int32_t expected = 0;
    const uint64_t deadline = LockFreeQueueTestControl::ticks() + 10000000000ULL;
    while ((!done.load() || !queue.empty()) && LockFreeQueueTestControl::ticks() < deadline)
    {
        if (queue.pop(&value))
        {
            correct = value == expected++ && correct;
        }
        queue.maintain(8);
    }
    stop.store(true);
    threads.join();
    LFQ_CHECK(correct && !failed.load() && expected == 5000 && queue.poolStatistics().peakReservedCapacity <= 16);
    return true;
}

bool LockFreeQueueTests::alignment()
{
    LockFreeQueueTestAligned::m_valid = true;
    LockFreeQueue<LockFreeQueueTestAligned> queue;
    LockFreeQueueTestAligned value(-1);
    for (int32_t i = 0; i < 2000; ++i)
    {
        value.m_value = i;
        queue.push(value);
    }
    for (int32_t i = 0; i < 2000; ++i)
    {
        LFQ_CHECK(queue.pop(&value) && value.m_value == i);
    }
    LFQ_CHECK(LockFreeQueueTestAligned::m_valid && queue.empty());
    LockFreeQueueEntry<LockFreeQueueTestAligned> entry(value);
    LFQ_CHECK(reinterpret_cast<uintptr_t>(&entry) % 64 == 0);
    for (size_t mode = 0; mode < 2; ++mode)
    {
        queue.destroy();
        LFQ_CHECK(queue.configureEntryOnly(mode != 0) == LockFreeQueueSuccess && queue.tryInit() == LockFreeQueueSuccess);
        for (int32_t i = 0; i < 128; ++i)
        {
            entry.m_value.m_value = i;
            LFQ_CHECK(queue.tryPushEntry(entry) == LockFreeQueueSuccess);
            LFQ_CHECK(queue.pop(&value) && value.m_value == i && queue.empty());
        }
    }
    LFQ_CHECK(LockFreeQueueTestAligned::m_valid);
    return true;
}

bool LockFreeQueueTests::pausedConsumer()
{
    const int32_t points[] = {4, 23, 7};
    for (size_t i = 0; i < 3; ++i)
    {
        LockFreeQueueTestControl::reset();
        LockFreeQueue<int32_t> queue;
        LFQ_CHECK(configure(queue, 2, 16));

        queue.setGrowPolicy([](const LockFreeQueueGrowContext&) { return size_t(2); });
        for (int32_t value = 0; value < 7; ++value)
        {
            queue.push(value);
        }
        if (points[i] == 7)
        {
            queue.clear();
            LockFreeQueueTestControl::m_clock.store(1000);
            queue.setShrinkPolicy(
                [](const LockFreeQueueShrinkContext&)
                {
                    LockFreeQueueShrinkDecision decision = {16, 10};
                    return decision;
                });
            queue.maintain(128);
            LockFreeQueueTestControl::m_clock.store(1009);
            queue.maintain(128);
            LFQ_CHECK(queue.poolStatistics().currentCapacity == 8);
            LockFreeQueueTestControl::m_clock.store(1010);
        }
        LockFreeQueueTestControl::m_pausePoint.store(points[i]);
        LockFreeQueueTestThreads threads;
        bool consumerOK = false;
        threads.m_threads.emplace_back(
            [&]()
            {
                g_lockFreeQueueTestLane = 1;
                if (points[i] == 7)
                {
                    consumerOK = queue.maintain(128).result == LockFreeQueueSuccess;
                }
                else
                {
                    int32_t value = -1;
                    consumerOK = queue.pop(&value) && value == 0;
                }
            });
        const bool reached = LockFreeQueueTestControl::waitReached();
        const uint64_t start = LockFreeQueueTestControl::ticks();
        const LockFreeQueueResult result = queue.tryPush(100);
        // Fill the active root block, then force traversal to the CLOSED block.
        const LockFreeQueueResult closedResult =
            points[i] == 7 ? queue.tryPush(101) : LockFreeQueueTemporaryUnavailable;
        const uint64_t elapsed = LockFreeQueueTestControl::ticks() - start;
        LockFreeQueueTestControl::m_release.store(true);
        threads.join();
        std::cout << "PAUSED_CONSUMER point=" << points[i] << " producer_result=" << result
                  << " producer_ns=" << elapsed << std::endl;
        LFQ_CHECK(reached && consumerOK && elapsed < 1000000000ULL);
        LFQ_CHECK(result == LockFreeQueueSuccess || result == LockFreeQueueBusy || result == LockFreeQueueResourceUnavailable);
        LFQ_CHECK(closedResult == LockFreeQueueBusy || closedResult == LockFreeQueueResourceUnavailable || closedResult == LockFreeQueueSuccess);
        if (points[i] != 7)
        {
            int32_t value = -1;
            for (int32_t expected = 1; expected < 7; ++expected)
            {
                LFQ_CHECK(queue.pop(&value) && value == expected);
            }
        }
        if (result == LockFreeQueueSuccess)
        {
            int32_t value = -1;
            LFQ_CHECK(queue.pop(&value) && value == 100);
        }
        if (closedResult == LockFreeQueueSuccess)
        {
            int32_t value = -1;
            LFQ_CHECK(queue.pop(&value) && value == 101);
        }
        LFQ_CHECK(queue.empty());
    }
    return true;
}

bool LockFreeQueueTests::steady(uint32_t seconds, bool producerSession, bool dynamicPool, bool prefixPool)
{
    // Bare std::thread control warms the CRT/COM runtime before queue measurement.
    // The separate diagnostic reproduced 45 persistent handles and delayed closes
    // without including Queue at all; comparing cold startup with joined is invalid.
    const uint32_t coldHandles = LockFreeQueueTestControl::handles();
    {
        std::atomic<bool> release(false);
        std::atomic<size_t> ready(0);
        LockFreeQueueThreadGroup warm(release);
        for (size_t i = 0; i < 8; ++i)
        {
            warm.m_threads.emplace_back(
                [&]()
                {
                    ready.fetch_add(1);
                    while (!release.load())
                    {
                        std::this_thread::yield();
                    }
                });
        }
        const uint64_t warmDeadline = LockFreeQueueTestControl::ticks() + 5000000000ULL;
        while (ready.load() != 8 && LockFreeQueueTestControl::ticks() < warmDeadline)
        {
            std::this_thread::yield();
        }
        release.store(true);
        warm.join();
        LFQ_CHECK(ready.load() == 8);
    }
    const uint32_t beforeHandles = LockFreeQueueTestControl::settledHandles();
    LFQ_CHECK(beforeHandles != UINT32_MAX);
    std::cout << "RESOURCE_CONTROL cold_handles=" << coldHandles << " warm_settled_handles=" << beforeHandles
              << std::endl;
    LockFreeQueue<uint64_t> queue;
    queue.destroy();
    LockFreeQueuePoolConfig config;
    config.initialCapacity = prefixPool ? 8192 : 256;
    config.maximumCapacity = prefixPool ? -1 : 256;
    config.growThreshold = dynamicPool ? 1 : 0;
    config.maximumProducers = 16;
    LFQ_CHECK(queue.configurePool(config) == LockFreeQueueSuccess && queue.tryInit() == LockFreeQueueSuccess);
    if (prefixPool)
    {
        // Keep repeated initial-cell reuse under contention, with no transition.
        // Unlimited capacity remains configured; an explicit policy declines
        // growth when all existing nodes are temporarily occupied.
        LFQ_CHECK(
            queue.setGrowPolicy([](const LockFreeQueueGrowContext&) { return size_t(0); }) == LockFreeQueueSuccess);
    }
    const LockFreeQueuePoolStatistics initialized = queue.poolStatistics();
    LFQ_CHECK(initialized.allocationCalls == 2);
    const size_t auxiliaryLimit = config.maximumProducers * 4 + 4;
    std::atomic<bool> start(false), stop(false), failed(false);
    std::atomic<size_t> done(0);
    std::atomic<uint64_t> accepted[8];
    uint64_t consumed[8] = {};
    std::atomic<uint64_t> retries(0);
    for (size_t lane = 0; lane < 8; ++lane)
    {
        accepted[lane].store(0);
    }
    LockFreeQueueThreadGroup group(stop, &start);
    for (size_t lane = 0; lane < 8; ++lane)
    {
        group.m_threads.emplace_back(
            [&, lane]()
            {
                LockFreeQueueProducer<uint64_t> producer;
                if (producerSession && queue.tryAcquireProducer(producer) != LockFreeQueueSuccess)
                {
                    failed.store(true);
                    stop.store(true);
                }
                while (!start.load())
                {
                    std::this_thread::yield();
                }
                uint64_t sequence = 0;
                while (!stop.load())
                {
                    const uint64_t value = (uint64_t(lane) << 56) | sequence;
                    const LockFreeQueueResult result = producerSession ? producer.tryPush(value) : queue.tryPush(value);
                    if (result == LockFreeQueueSuccess)
                    {
                        accepted[lane].store(++sequence);
                    }
                    else if (result == LockFreeQueueCapacityLimit || LockFreeQueueBenchmarkPush::transient(result) ||
                             (prefixPool && result == LockFreeQueueGrowthDeclined))
                    {
                        retries.fetch_add(1);
                        std::this_thread::yield();
                    }
                    else
                    {
                        failed.store(true);
                        stop.store(true);
                    }
                }
                producer.release();
                done.fetch_add(1);
            });
    }
    const uint64_t began = LockFreeQueueTestControl::ticks();
    const uint64_t deadline = began + uint64_t(seconds) * 1000000000ULL;
    uint64_t nextObservation = began;
    uint64_t received = 0;
    bool valid = true;
    start.store(true);
    while ((done.load() != 8 || !queue.empty()) && LockFreeQueueTestControl::ticks() < deadline + 10000000000ULL)
    {
        const uint64_t current = LockFreeQueueTestControl::ticks();
        if (current >= deadline)
        {
            stop.store(true);
        }
        uint64_t value = 0;
        if (queue.pop(&value))
        {
            const size_t lane = static_cast<size_t>(value >> 56);
            valid = lane < 8 && (value & 0x00ffffffffffffffULL) == consumed[lane]++ && valid;
            ++received;
        }
        else
        {
            std::this_thread::yield();
        }
        if (current >= nextObservation)
        {
            const LockFreeQueuePoolStatistics snapshot = queue.poolStatistics();
            std::cout << "STEADY_SAMPLE elapsed_ms=" << (current - began) / 1000000 << " received=" << received
                      << " capacity=" << snapshot.currentCapacity << " retained_bytes=" << snapshot.retainedBytes
                      << " allocations=" << snapshot.allocationCalls
                      << " private_bytes=" << LockFreeQueueTestControl::privateBytes()
                      << " handles=" << LockFreeQueueTestControl::handles() << std::endl;
            // Payload capacity stays fixed. Publication metadata may be claimed
            // lazily when paused producers pin older segments. Account for every
            // allocation and byte against the independent, finite registry.
            valid = snapshot.peakReservedCapacity == config.initialCapacity && snapshot.successfulGrowCount == 0 &&
                    snapshot.currentCapacity == config.initialCapacity && snapshot.reservedCapacity == 0 &&
                    snapshot.allocationCalls >= initialized.allocationCalls &&
                    snapshot.allocationCalls - initialized.allocationCalls <= auxiliaryLimit &&
                    snapshot.retainedBytes >= initialized.retainedBytes &&
                    snapshot.retainedBytes <= initialized.retainedBytes +
                        auxiliaryLimit * sizeof(LockFreeQueueArena<uint64_t>::Segment) && valid;
            nextObservation = current + 10000000000ULL;
        }
    }
    stop.store(true);
    group.join();
    for (size_t lane = 0; lane < 8; ++lane)
    {
        valid = consumed[lane] == accepted[lane].load() && consumed[lane] != 0 && valid;
    }
    valid = queue.maintain(2048).result == LockFreeQueueSuccess && valid;
    const LockFreeQueuePoolStatistics snapshot = queue.poolStatistics();
    // Concurrent statistics are weak snapshots. Check the exact allocation/byte
    // relationship only after joining every producer, with no outstanding claim.
    valid = snapshot.allocationCalls >= initialized.allocationCalls &&
            snapshot.allocationCalls - initialized.allocationCalls <= auxiliaryLimit &&
            snapshot.retainedBytes == initialized.retainedBytes +
                (snapshot.allocationCalls - initialized.allocationCalls) *
                    sizeof(LockFreeQueueArena<uint64_t>::Segment) && valid;
    queue.destroy();
    const uint32_t immediateHandles = LockFreeQueueTestControl::handles();
    const uint32_t afterHandles = LockFreeQueueTestControl::settledHandles();
    std::cout << "STEADY producer_context=" << producerSession << " seconds=" << seconds << " received=" << received
              << " retry_refusals=" << retries.load() << " cas_retries=" << snapshot.casRetries
              << " before_handles=" << beforeHandles << " immediate_handles=" << immediateHandles
              << " after_handles=" << afterHandles
              << " data_and_resource_valid=" << valid << " producer_failed=" << failed.load()
              << " used_nodes=" << snapshot.usedNodes << " free_nodes=" << snapshot.reusableFreeCapacity
              << " retired_nodes=" << snapshot.retiredNodes << " active_producers=" << snapshot.activeProducers
              << " allocations=" << snapshot.allocationCalls << " retained_bytes=" << snapshot.retainedBytes
              << " final_private_bytes=" << LockFreeQueueTestControl::privateBytes() << " dynamic_pool=" << dynamicPool
              << " prefix_pool=" << prefixPool << std::endl;
    LFQ_CHECK(valid && !failed.load() && snapshot.usedNodes == 1 && snapshot.retiredNodes == 0 &&
              snapshot.activeProducers == 0 && snapshot.reusableFreeCapacity == config.initialCapacity - 1 &&
              snapshot.reservedCapacity == 0 && beforeHandles == afterHandles);
    return true;
}

bool LockFreeQueueTests::cacheReuse()
{
    LockFreeQueue<int32_t> queue;
    LFQ_CHECK(configure(queue, 9, 9, 0, 16));
    for (int32_t round = 0; round < 2000; ++round)
    {
        for (int32_t i = 0; i < 8; ++i)
        {
            LFQ_CHECK(queue.tryPush(round * 8 + i) == LockFreeQueueSuccess);
        }
        LFQ_CHECK(queue.tryPush(-1) == LockFreeQueueCapacityLimit);
        for (int32_t i = 0; i < 8; ++i)
        {
            int32_t value = -1;
            LFQ_CHECK(queue.pop(&value) && value == round * 8 + i);
        }
        const LockFreeQueuePoolStatistics stats = queue.poolStatistics();
        LFQ_CHECK(stats.usedNodes == 1 && stats.reusableFreeCapacity == 8 && stats.currentCapacity == 9 &&
                  stats.peakReservedCapacity == 9 && stats.allocationCalls == 2);
    }
    return mpsc(16, 5000, true);
}

bool LockFreeQueueTests::cacheInterleavings()
{
    LockFreeQueue<int32_t> queue;
    LFQ_CHECK(configure(queue, 2, 16));

    queue.setGrowPolicy([](const LockFreeQueueGrowContext&) { return size_t(2); });
    queue.push(0);
    LFQ_CHECK(queue.pop(nullptr));
    queue.setShrinkPolicy(
        [](const LockFreeQueueShrinkContext&)
        {
            LockFreeQueueShrinkDecision decision = {16, 1};
            return decision;
        });
    LockFreeQueueTestControl::m_pausePoint.store(13);
    LockFreeQueueTestThreads threads;
    std::atomic<int32_t> pushed(-1);
    threads.m_threads.emplace_back(
        [&]()
        {
            g_lockFreeQueueTestLane = 1;
            pushed.store(queue.tryPush(99));
        });
    bool valid = LockFreeQueueTestControl::waitReached();
    for (int32_t i = 0; i < 200; ++i)
    {
        const LockFreeQueueResult result = queue.tryPush(i);
        int32_t value = -1;
        valid = result == LockFreeQueueSuccess && queue.pop(&value) && value == i && valid;
        queue.maintain(8);
    }
    valid = queue.poolStatistics().usedNodes >= 2 && queue.poolStatistics().peakReservedCapacity <= 16 && valid;
    LockFreeQueueTestControl::m_release.store(true);
    threads.join();
    int32_t value = -1;
    LFQ_CHECK(valid && pushed.load() == LockFreeQueueSuccess && queue.pop(&value) && value == 99 && queue.empty());

    LockFreeQueueTestControl::reset();
    LockFreeQueue<int32_t> duringGrowth;
    LFQ_CHECK(configure(duringGrowth, 2, 3));
    duringGrowth.setGrowPolicy(
        [](const LockFreeQueueGrowContext&)
        {
            LockFreeQueueTestHook(14, nullptr);
            return size_t(0);
        });
    duringGrowth.push(7);
    LockFreeQueueTestControl::m_pausePoint.store(14);
    LockFreeQueueTestThreads growthThreads;
    std::atomic<int32_t> growthResult(-1);
    growthThreads.m_threads.emplace_back(
        [&]()
        {
            g_lockFreeQueueTestLane = 1;
            growthResult.store(duringGrowth.tryPush(8));
        });
    valid = LockFreeQueueTestControl::waitReached();
    valid = duringGrowth.pop(&value) && value == 7 && valid;
    LockFreeQueueTestControl::m_release.store(true);
    growthThreads.join();
    LFQ_CHECK(valid && growthResult.load() == LockFreeQueueSuccess && duringGrowth.pop(&value) && value == 8 &&
              duringGrowth.empty());
    LFQ_CHECK(duringGrowth.poolStatistics().allocationCalls == 2 && duringGrowth.poolStatistics().currentCapacity == 2);
    LockFreeQueueTestControl::reset();
    LockFreeQueue<int32_t> smallBudget;
    LFQ_CHECK(configure(smallBudget, 2, 16));
    smallBudget.setGrowPolicy([](const LockFreeQueueGrowContext&) { return size_t(2); });
    for (int32_t i = 0; i < 7; ++i)
    {
        smallBudget.push(i);
    }
    smallBudget.clear();
    smallBudget.setShrinkPolicy(
        [](const LockFreeQueueShrinkContext&)
        {
            LockFreeQueueShrinkDecision decision = {16, 10};
            return decision;
        });
    LockFreeQueueTestControl::m_clock.store(1000);
    for (size_t i = 0; i < 1024; ++i)
    {
        LFQ_CHECK(smallBudget.maintain(1).result == LockFreeQueueSuccess);
    }
    LFQ_CHECK(smallBudget.poolStatistics().currentCapacity == 8);
    LockFreeQueueTestControl::m_clock.store(1010);
    for (size_t i = 0; i < 1024; ++i)
    {
        LFQ_CHECK(smallBudget.maintain(1).result == LockFreeQueueSuccess);
    }
    LFQ_CHECK(smallBudget.poolStatistics().currentCapacity == 2);
    for (int32_t i = 0; i < 7; ++i)
    {
        smallBudget.push(i);
    }
    for (int32_t i = 0; i < 7; ++i)
    {
        LFQ_CHECK(smallBudget.pop(&value) && value == i);
    }
    return true;
}

bool LockFreeQueueTests::counters()
{
    // Independent physical oracle across size_t wrap and cross-record returns.
    // No payload publication is involved: exactly 65 bitmap nodes are available.
    {
        LockFreeQueueArena<int32_t> pool;
        LockFreeQueuePoolConfig physical;
        physical.initialCapacity = 66;
        physical.maximumCapacity = -1;
        physical.maximumProducers = 2;
        LFQ_CHECK(pool.configure(physical) == LockFreeQueueSuccess && pool.initialize() == LockFreeQueueSuccess);
        LockFreeQueueArena<int32_t>::Record* first = pool.enter();
        LockFreeQueueArena<int32_t>::Record* second = pool.enter();
        LFQ_CHECK(first != nullptr && second != nullptr);
        pool.testSeedInitialAccounting((std::numeric_limits<size_t>::max)() - 7);
        LockFreeQueueArena<int32_t>::Node* nodes[65];
        for (size_t lap = 0; lap < 100; ++lap)
        {
            for (size_t i = 0; i < 65; ++i)
            {
                LockFreeQueueResult status = LockFreeQueueTemporaryUnavailable;
                nodes[i] = pool.acquireNode(i % 2 == 0 ? *first : *second, status);
                pool.releaseCache(*first);
                pool.releaseCache(*second);
                LFQ_CHECK(nodes[i] != nullptr && status == LockFreeQueueSuccess);
                LFQ_CHECK(pool.statistics().usedNodes == i + 2);
            }
            for (size_t i = 0; i < 65; ++i)
            {
                // Returns deliberately use the opposite lease or the consumer.
                pool.releaseNode(nodes[i], i % 3 == 0 ? nullptr : (i % 2 == 0 ? second : first));
                LFQ_CHECK(pool.statistics().usedNodes == 65 - i);
            }
            LFQ_CHECK(pool.statistics().reusableFreeCapacity == 65 && pool.statistics().allocationCalls == 2);
        }
        std::cout << "POOL_COUNTER_WRAP laps=100 physical_nodes=65 cross_record_returns=passed" << std::endl;
    }
    LockFreeQueue<uint64_t> queue;
    queue.destroy();
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 256;
    config.maximumCapacity = 256;
    config.maximumProducers = 16;
    LFQ_CHECK(queue.configurePool(config) == LockFreeQueueSuccess && queue.tryInit() == LockFreeQueueSuccess);
    queue.testSeedCounters(UINT64_MAX - 7);
    for (uint64_t i = 0; i < 32; ++i)
    {
        queue.push(i);
        LFQ_CHECK(queue.size() == i + 1 && !queue.empty());
    }
    for (uint64_t i = 0; i < 32; ++i)
    {
        uint64_t value = UINT64_MAX;
        LFQ_CHECK(queue.pop(&value) && value == i && queue.size() == 31 - i);
    }
    LFQ_CHECK(queue.empty());
    queue.testSeedCounters(UINT64_MAX - 1);
    LockFreeQueueTestControl::m_pausePoint.store(2);
    LockFreeQueueTestThreads paused;
    std::atomic<int32_t> result(-1);
    paused.m_threads.emplace_back(
        [&]()
        {
            g_lockFreeQueueTestLane = 1;
            LockFreeQueueResult attempt = queue.tryPush(10);
            if (attempt == LockFreeQueueTemporaryUnavailable)
            {
                attempt = queue.push(10);
            }
            result.store(attempt);
        });
    bool valid = LockFreeQueueTestControl::waitReached();
    valid = queue.tryPush(20) == LockFreeQueueSuccess && valid;
    valid = queue.size() == 2 && valid;
    uint64_t value = 0;
    valid = queue.pop(&value) && value == 20 && queue.size() == 1 && valid;
    LockFreeQueueTestControl::m_release.store(true);
    paused.join();
    LFQ_CHECK(valid && result.load() == LockFreeQueueSuccess && queue.pop(&value) && value == 10 && queue.empty());
    LockFreeQueueTestControl::reset();
    queue.testSeedCounters(UINT64_MAX - 100000);
    std::atomic<bool> start(false), stop(false), error(false);
    std::atomic<size_t> done(0);
    std::atomic<uint64_t> observations(0);
    LockFreeQueueThreadGroup group(stop, &start);
    const size_t producers = 4;
    const size_t perProducer = 30000;
    for (size_t lane = 0; lane < producers; ++lane)
    {
        group.m_threads.emplace_back(
            [&, lane]()
            {
                while (!start.load())
                {
                    std::this_thread::yield();
                }
                for (size_t i = 0; i < perProducer && !stop.load(); ++i)
                {
                    const uint64_t item = (uint64_t(lane) << 32) | i;
                    while (!stop.load())
                    {
                        const LockFreeQueueResult pushed = queue.tryPush(item);
                        if (pushed == LockFreeQueueSuccess)
                        {
                            break;
                        }
                        if (pushed != LockFreeQueueCapacityLimit && !LockFreeQueueBenchmarkPush::transient(pushed))
                        {
                            error.store(true);
                            stop.store(true);
                        }
                        std::this_thread::yield();
                    }
                }
                done.fetch_add(1);
            });
    }
    group.m_threads.emplace_back(
        [&]()
        {
            while (!start.load())
            {
                std::this_thread::yield();
            }
            uint64_t sampled = 0;
            do
            {
                // This run can add at most 120000, so a larger snapshot would be
                // an underflow, mixed-epoch bug or count corruption, not valid drift.
                if (queue.size() > producers * perProducer)
                {
                    error.store(true);
                    stop.store(true);
                }
                ++sampled;
            } while (!stop.load());
            observations.store(sampled);
        });
    std::vector<uint64_t> expected(producers, 0);
    size_t received = 0;
    const uint64_t deadline = LockFreeQueueTestControl::ticks() + 15000000000ULL;
    start.store(true);
    while (received != producers * perProducer && !stop.load() && LockFreeQueueTestControl::ticks() < deadline)
    {
        if (queue.pop(&value))
        {
            const size_t lane = static_cast<size_t>(value >> 32);
            if (lane >= producers || uint32_t(value) != expected[lane]++)
            {
                error.store(true);
                stop.store(true);
            }
            ++received;
        }
        else
        {
            std::this_thread::yield();
        }
    }
    stop.store(true);
    group.join();
    std::cout << "COUNTERS received=" << received << " observations=" << observations.load()
              << " final_size=" << queue.size() << " producer_done=" << done.load() << std::endl;
    LFQ_CHECK(!error.load() && received == producers * perProducer && done.load() == producers &&
              observations.load() != 0 && queue.size() == 0 && queue.empty());
    queue.destroy();
    LFQ_CHECK(queue.size() == 0 && queue.tryInit() == LockFreeQueueSuccess && queue.empty());
    queue.push(123);
    return queue.size() == 1 && queue.pop(&value) && value == 123 && queue.empty();
}

bool LockFreeQueueTests::segmentReuse()
{
    LockFreeQueue<int32_t> queue;
    LFQ_CHECK(configure(queue, 3, 3, 0, 2));
    LockFreeQueueTestControl::m_pausePoint.store(1);
    LockFreeQueueTestThreads threads;
    std::atomic<int32_t> result(-1);
    threads.m_threads.emplace_back(
        [&]()
        {
            g_lockFreeQueueTestLane = 1;
            result.store(queue.tryPush(111));
        });
    bool valid = LockFreeQueueTestControl::waitReached();
    // Publisher holds an old numeric slot snapshot while the other node and
    // ring cells are repeatedly reused. It must not retain or read that T.
    for (int32_t i = 0; i < 10000; ++i)
    {
        int32_t value = -1;
        valid = queue.tryPush(i) == LockFreeQueueSuccess && valid;
        valid = queue.pop(&value) && value == i && valid;
    }
    const LockFreeQueuePoolStatistics paused = queue.poolStatistics();
    valid = paused.usedNodes == 2 && paused.retiredNodes == 0 && paused.reusableFreeCapacity == 1 &&
            paused.allocationCalls == 2 && valid;
    LockFreeQueueTestControl::m_release.store(true);
    threads.join();
    int32_t value = -1;
    LFQ_CHECK(valid && result.load() == LockFreeQueueSuccess && queue.pop(&value) && value == 111 && queue.empty());
    for (int32_t point = 10; point <= 11; ++point)
    {
        queue.destroy();
        LockFreeQueueTestControl::m_failPoint.store(point);
        LFQ_CHECK(queue.tryInit() == LockFreeQueueAllocationFailure);
        LFQ_CHECK(queue.tryPush(7) == LockFreeQueueNotInitialized && queue.poolStatistics().currentCapacity == 0);
        LFQ_CHECK(queue.tryInit() == LockFreeQueueSuccess && queue.tryPush(7) == LockFreeQueueSuccess);
        LFQ_CHECK(queue.pop(&value) && value == 7);
    }
    LFQ_CHECK(configure(queue, 2, 2, 0, 2));
    LockFreeQueueTestControl::reset();
    LockFreeQueueTestControl::m_pausePoint.store(3);
    threads.m_threads.emplace_back(
        [&]()
        {
            g_lockFreeQueueTestLane = 1;
            result.store(queue.tryPush(91));
        });
    valid = LockFreeQueueTestControl::waitReached();
    valid = queue.pop(&value) && value == 91 && valid;
    valid = queue.tryPush(92) == LockFreeQueueSuccess && valid;
    valid = queue.pop(&value) && value == 92 && valid;
    LockFreeQueueTestControl::m_release.store(true);
    threads.join();
    LFQ_CHECK(valid && result.load() == LockFreeQueueSuccess && queue.empty());
    LockFreeQueueTestControl::reset();
    LFQ_CHECK(queue.tryInit() == LockFreeQueueSuccess);
    queue.testSeedGeneration(UINT64_MAX - 1);
    for (int32_t index = 0; index < 512; ++index)
    {
        LFQ_CHECK(queue.tryPush(index) == LockFreeQueueSuccess && queue.pop(&value) && value == index);
    }
    LFQ_CHECK(queue.tryPush(82) == LockFreeQueueSequenceExhausted && queue.empty());
    LFQ_CHECK(queue.push(83) == LockFreeQueueSequenceExhausted && queue.empty());
    LFQ_CHECK(queue.tryInit() == LockFreeQueueSuccess && queue.tryPush(84) == LockFreeQueueSuccess);
    LFQ_CHECK(queue.pop(&value) && value == 84);
    // A paused publisher keeps a private hint while another reaches exhaustion.
    // Resumption must cancel its own unpublished payload and count, without wrap.
    LFQ_CHECK(configure(queue, 3, 3, 0, 2));
    queue.testSeedGeneration(UINT64_MAX - 1);
    LockFreeQueueTestControl::reset();
    LockFreeQueueTestControl::m_pausePoint.store(2);
    result.store(-1);
    threads.m_threads.emplace_back(
        [&]()
        {
            g_lockFreeQueueTestLane = 1;
            const LockFreeQueueResult attempted = queue.tryPush(999);
            result.store(attempted == LockFreeQueueTemporaryUnavailable ? queue.push(999) : attempted);
        });
    valid = LockFreeQueueTestControl::waitReached();
    size_t accepted = 0;
    LockFreeQueueResult stopped = LockFreeQueueSuccess;
    for (; accepted < 1024; ++accepted)
    {
        stopped = queue.tryPush(static_cast<int32_t>(accepted));
        if (stopped != LockFreeQueueSuccess)
        {
            break;
        }
        valid = queue.pop(&value) && value == static_cast<int32_t>(accepted) && valid;
    }
    valid = stopped == LockFreeQueueSequenceExhausted && accepted != 0 && queue.size() == 1 && valid;
    LockFreeQueueTestControl::m_release.store(true);
    threads.join();
    LFQ_CHECK(valid && result.load() == LockFreeQueueSequenceExhausted && queue.empty() && !queue.pop(&value));
    LFQ_CHECK(queue.poolStatistics().usedNodes == 1 && queue.tryInit() == LockFreeQueueSuccess);
    std::cout << "SEGMENT_LATE_EXHAUSTION accepted=" << accepted << " unpublished_rollback=passed" << std::endl;
    std::cout << "SEGMENT_REUSE laps=2500 stale_publish=preserved sequence_exhaustion=uncommitted recovery=passed"
              << std::endl;
    return true;
}

bool LockFreeQueueTests::cachedBatches()
{
    LockFreeQueue<int32_t> queue;
    LFQ_CHECK(configure(queue, 2049, 2049, 0, 8));
    LockFreeQueueTestControl::m_pausePoint.store(15);
    LockFreeQueueTestThreads threads;
    std::atomic<int32_t> first(-1);
    threads.m_threads.emplace_back(
        [&]()
        {
            g_lockFreeQueueTestLane = 1;
            first.store(queue.tryPush(99999));
        });
    bool valid = LockFreeQueueTestControl::waitReached();
    size_t accepted = 0;
    LockFreeQueueResult refused = LockFreeQueueSuccess;
    for (; accepted < 2048; ++accepted)
    {
        refused = queue.tryPush(static_cast<int32_t>(accepted));
        if (refused != LockFreeQueueSuccess)
        {
            break;
        }
    }
    // Node zero removes one bit from the first native-width batch.
    const size_t heldBatch = 31;
    valid = accepted == 2048 - heldBatch && refused == LockFreeQueueCapacityLimit && valid;
    const LockFreeQueuePoolStatistics held = queue.poolStatistics();
    valid = held.currentCapacity == 2049 && held.peakReservedCapacity == 2049 && held.allocationCalls == 2 &&
            held.reusableFreeCapacity == 0 && valid;
    for (size_t index = 0; index < accepted; ++index)
    {
        int32_t value = -1;
        valid = queue.pop(&value) && value == static_cast<int32_t>(index) && valid;
    }
    LockFreeQueueTestControl::m_release.store(true);
    threads.join();
    int32_t value = -1;
    LFQ_CHECK(valid && first.load() == LockFreeQueueSuccess && queue.pop(&value) && value == 99999);
    queue.maintain(4096);
    const LockFreeQueuePoolStatistics idle = queue.poolStatistics();
    LFQ_CHECK(idle.usedNodes == 1 && idle.reusableFreeCapacity == 2048);
    for (int32_t index = 0; index < 2048; ++index)
    {
        LFQ_CHECK(queue.tryPush(index) == LockFreeQueueSuccess);
    }
    LFQ_CHECK(queue.tryPush(-1) == LockFreeQueueCapacityLimit);
    for (int32_t index = 0; index < 2048; ++index)
    {
        LFQ_CHECK(queue.pop(&value) && value == index);
    }
    LFQ_CHECK(queue.empty() && queue.poolStatistics().allocationCalls == 2);
    // Four-node batches cross the last bit and sparse returned-node words.
    LFQ_CHECK(configure(queue, 65, 65, 0, 4));
    int32_t submitted = 0, received = 0;
    for (size_t index = 0; index < 64; ++index)
    {
        LFQ_CHECK(queue.tryPush(submitted++) == LockFreeQueueSuccess);
    }
    for (size_t round = 0; round < 1000; ++round)
    {
        for (size_t index = 0; index < 17; ++index)
        {
            LFQ_CHECK(queue.pop(&value) && value == received++);
        }
        for (size_t index = 0; index < 17; ++index)
        {
            LFQ_CHECK(queue.tryPush(submitted++) == LockFreeQueueSuccess);
        }
        LFQ_CHECK(queue.tryPush(-1) == LockFreeQueueCapacityLimit && queue.size() == 64);
    }
    while (queue.pop(&value))
    {
        LFQ_CHECK(value == received++);
    }
    LFQ_CHECK(received == submitted && queue.poolStatistics().allocationCalls == 2);
    std::cout << "SEGMENT_CACHED private_batch=" << heldBatch << " other_accepted=" << accepted
              << " full_capacity_reused=2048 allocations=" << queue.poolStatistics().allocationCalls << std::endl;
    return true;
}

bool LockFreeQueueTests::capacityContracts()
{
    // A Win32 Release stack aggregate previously had only four-byte alignment.
    // Check actual addresses: alignment_of alone did not detect that failure.
    LockFreeQueue<int32_t> alignedQueues[2];
    std::unique_ptr<LockFreeQueue<int32_t>> heapQueue(new LockFreeQueue<int32_t>());
    LFQ_CHECK(reinterpret_cast<uintptr_t>(&alignedQueues[0]) % 8 == 0 &&
              reinterpret_cast<uintptr_t>(&alignedQueues[1]) % 8 == 0 &&
              reinterpret_cast<uintptr_t>(heapQueue.get()) % 8 == 0);
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 8;
    config.maximumCapacity = 8;
    {
        LockFreeQueue<LockFreeQueueTestValue> queue;
        queue.destroy();
        LFQ_CHECK(queue.configurePool(config) == LockFreeQueueSuccess && queue.tryInit() == LockFreeQueueSuccess);
        LockFreeQueueTestValue source(42), output(0);
        LockFreeQueueTestValue::m_throwCopy = true;
        const LockFreeQueueResult failed = queue.tryPush(source);
        bool caught = false;
        const LockFreeQueueResult pushed = queue.push(source);
        LockFreeQueueTestValue::m_throwCopy = false;
        LFQ_CHECK(failed == LockFreeQueueConstructionFailure && pushed == LockFreeQueueConstructionFailure && queue.empty() &&
                  queue.poolStatistics().usedNodes == 1);
        queue.push(source);
        LockFreeQueueTestValue::m_throwAssign = true;
        caught = false;
        try
        {
            queue.pop(&output);
        }
        catch (const std::runtime_error&)
        {
            caught = true;
        }
        LockFreeQueueTestValue::m_throwAssign = false;
        LFQ_CHECK(caught && output.m_value == -999 && queue.size() == 1);
        LFQ_CHECK(queue.pop(&output) && output.m_value == 42);
    }
    LFQ_CHECK(LockFreeQueueTestValue::m_live.load() == 0);
    {
        LockFreeQueue<LockFreeQueueTestAligned> queue;
        queue.destroy();
        LFQ_CHECK(queue.configurePool(config) == LockFreeQueueSuccess && queue.tryInit() == LockFreeQueueSuccess);
        LockFreeQueueTestAligned::m_valid = true;
        LockFreeQueueTestAligned value(0);
        for (int32_t i = 0; i < 2000; ++i)
        {
            value.m_value = i;
            queue.push(value);
            LFQ_CHECK(queue.pop(&value) && value.m_value == i);
        }
        LFQ_CHECK(LockFreeQueueTestAligned::m_valid);
    }
    {
        LockFreeQueue<std::shared_ptr<LockFreeQueueTestValue>> queue;
        queue.destroy();
        LFQ_CHECK(queue.configurePool(config) == LockFreeQueueSuccess && queue.tryInit() == LockFreeQueueSuccess);
        std::shared_ptr<LockFreeQueueTestValue> value(new LockFreeQueueTestValue(42));
        std::weak_ptr<LockFreeQueueTestValue> weak(value);
        queue.push(value);
        value.reset();
        LFQ_CHECK(!weak.expired() && queue.pop(nullptr) && weak.expired());
    }
    std::weak_ptr<LockFreeQueueTestValue> queuedWeak;
    {
        LockFreeQueue<std::shared_ptr<LockFreeQueueTestValue>> queued;
        queued.destroy();
        LFQ_CHECK(queued.configurePool(config) == LockFreeQueueSuccess && queued.tryInit() == LockFreeQueueSuccess);
        std::shared_ptr<LockFreeQueueTestValue> payload(new LockFreeQueueTestValue(51));
        queuedWeak = payload;
        queued.push(payload);
        queued.push(payload);
        payload.reset();
        queued.destroy();
        LFQ_CHECK(queuedWeak.expired() && queued.tryInit() == LockFreeQueueSuccess);
        payload.reset(new LockFreeQueueTestValue(52));
        queuedWeak = payload;
        queued.push(payload);
        payload.reset();
        LFQ_CHECK(!queuedWeak.expired());
    }
    LFQ_CHECK(queuedWeak.expired() && LockFreeQueueTestValue::m_live.load() == 0);
    LockFreeQueue<int32_t> queue;
    int32_t observed = 0, privateCounter = 0;
    queue.setGrowPolicy(
        [&observed, privateCounter](const LockFreeQueueGrowContext&) mutable
        {
            observed = ++privateCounter;
            return size_t(0);
        });
    for (int32_t phase = 0; phase < 3; ++phase)
    {
        LFQ_CHECK(configure(queue, 2, phase == 1 ? 3 : 2));
        queue.push(7);
        LFQ_CHECK(queue.tryPush(8) == (phase == 1 ? LockFreeQueueGrowthDeclined : LockFreeQueueCapacityLimit));
        LFQ_CHECK(observed == (phase == 0 ? 0 : 1));
        queue.clear();
    }
    queue.destroy();
    config.initialCapacity = 2;
    config.maximumCapacity = INT64_MAX;
    LFQ_CHECK(queue.configurePool(config) == LockFreeQueueInvalidConfiguration);
    LFQ_CHECK(queue.tryInit() == LockFreeQueueSuccess && queue.poolStatistics().currentCapacity == 2);
    queue.setShrinkPolicy(
        [](const LockFreeQueueShrinkContext&) -> LockFreeQueueShrinkDecision { throw std::runtime_error("shrink"); });
    LFQ_CHECK(queue.maintain(1).result == LockFreeQueuePolicyFailure);
    LockFreeQueueTestControl::m_clock.store(0);
    const LockFreeQueueResult clock = queue.maintain(1).result;
    LockFreeQueueTestControl::m_clock.store(UINT64_MAX);
    LFQ_CHECK(clock == LockFreeQueueClockUnavailable && queue.maintain(0).result == LockFreeQueueSuccess);
    LFQ_CHECK(LockFreeQueueTestValue::m_live.load() == 0);
    return true;
}

bool LockFreeQueueTests::reservedProducer()
{
    for (size_t mode = 0; mode < 2; ++mode)
    {
        LockFreeQueue<int32_t> queue;
        LFQ_CHECK(configure(queue, 8, mode == 0 ? 8 : -1, 0, 2));
        LockFreeQueueProducer<int32_t> first, second, extra;
        LFQ_CHECK(!first.attached() && first.tryPush(1) == LockFreeQueueNotInitialized);
        LFQ_CHECK(first.push(1) == LockFreeQueueNotInitialized);
        LFQ_CHECK(queue.tryAcquireProducer(first) == LockFreeQueueSuccess && first.attached());
        LFQ_CHECK(queue.tryAcquireProducer(first) == LockFreeQueueInvalidState && first.attached());
        LFQ_CHECK(queue.tryAcquireProducer(second) == LockFreeQueueSuccess);
        LFQ_CHECK(queue.tryAcquireProducer(extra) == LockFreeQueueCapacityLimit && !extra.attached());
        LockFreeQueue<int32_t> other;
        LFQ_CHECK(other.tryAcquireProducer(first) == LockFreeQueueInvalidState);
        for (int32_t value = 0; value < 10000; ++value)
        {
            int32_t output = -1;
            (value % 2 == 0 ? first : second).push(value);
            LFQ_CHECK(queue.pop(&output) && output == value);
        }
        if (mode != 0)
        {
            for (int32_t index = 0; index < 70; ++index)
            {
                first.push(index);
            }
            const size_t peak = queue.poolStatistics().currentCapacity;
            for (int32_t index = 0; index < 70; ++index)
            {
                int32_t output = -1;
                LFQ_CHECK(queue.pop(&output) && output == index);
            }
            queue.setShrinkPolicy(
                [](const LockFreeQueueShrinkContext&)
                {
                    const LockFreeQueueShrinkDecision decision = {1024, 1};
                    return decision;
                });
            queue.maintain(4096);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            queue.maintain(4096);
            queue.maintain(4096);
            LFQ_CHECK(queue.poolStatistics().currentCapacity < peak && queue.poolStatistics().activeProducers == 2);
        }
        first.release();
        first.release();
        LFQ_CHECK(!first.attached() && queue.tryPush(77) == LockFreeQueueSuccess);
        int32_t value = -1;
        LFQ_CHECK(queue.pop(&value) && value == 77);
        second.release();
        LFQ_CHECK(queue.poolStatistics().activeProducers == 0);
        queue.destroy();
        LFQ_CHECK(queue.tryAcquireProducer(extra) == LockFreeQueueNotInitialized);
        LFQ_CHECK(queue.tryInit() == LockFreeQueueSuccess && queue.tryAcquireProducer(extra) == LockFreeQueueSuccess);
        extra.release();
    }
    LockFreeQueue<int32_t> queue;
    LFQ_CHECK(configure(queue, 2049, 2049, 0, 8));
    LockFreeQueueProducer<int32_t> first, producer;
    LFQ_CHECK(queue.tryAcquireProducer(first) == LockFreeQueueSuccess &&
              queue.tryAcquireProducer(producer) == LockFreeQueueSuccess);
    LFQ_CHECK(first.tryPush(0) == LockFreeQueueSuccess);
    first.release();
    for (int32_t value = 1; value < 2048; ++value)
    {
        LFQ_CHECK(producer.tryPush(value) == LockFreeQueueSuccess);
    }
    LFQ_CHECK(producer.tryPush(-1) == LockFreeQueueCapacityLimit && queue.poolStatistics().activeProducers == 1);
    for (int32_t expected = 0; expected < 2048; ++expected)
    {
        int32_t value = -1;
        LFQ_CHECK(queue.pop(&value) && value == expected);
    }
    producer.release();
    LFQ_CHECK(queue.poolStatistics().activeProducers == 0 && queue.poolStatistics().allocationCalls == 2);
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    LFQ_CHECK(queue.tryAcquireProducer(first) == LockFreeQueueSuccess &&
              queue.tryAcquireProducer(producer) == LockFreeQueueSuccess);
    LockFreeQueueTestControl::reset();
    LockFreeQueueTestControl::m_pausePoint.store(2);
    std::atomic<int32_t> result(-1);
    LockFreeQueueTestThreads workers;
    workers.m_threads.emplace_back(
        [&]()
        {
            g_lockFreeQueueTestLane = 1;
            const LockFreeQueueResult attempted = first.tryPush(111);
            result.store(attempted == LockFreeQueueTemporaryUnavailable ? first.push(111) : attempted);
        });
    bool valid = LockFreeQueueTestControl::waitReached();
    valid = producer.tryPush(222) == LockFreeQueueSuccess && valid;
    int32_t value = 0;
    valid = queue.pop(&value) && value == 222 && valid;
    LockFreeQueueTestControl::m_release.store(true);
    workers.join();
    valid = result.load() == LockFreeQueueSuccess && queue.pop(&value) && value == 111 && valid;
    first.release();
    producer.release();
    LFQ_CHECK(valid && queue.size() == 0 && queue.poolStatistics().activeProducers == 0);
    LockFreeQueueTestControl::reset();
#endif
    for (size_t mode = 0; mode < 2; ++mode)
    {
        LockFreeQueue<LockFreeQueueTestValue> values;
        values.destroy();
        LockFreeQueuePoolConfig config;
        config.initialCapacity = 8;
        config.maximumCapacity = mode == 0 ? 8 : -1;
        LFQ_CHECK(values.configurePool(config) == LockFreeQueueSuccess && values.tryInit() == LockFreeQueueSuccess);
        {
            LockFreeQueueProducer<LockFreeQueueTestValue> session;
            LFQ_CHECK(values.tryAcquireProducer(session) == LockFreeQueueSuccess);
            LockFreeQueueTestValue input(42), output(0);
            LockFreeQueueTestValue::m_throwCopy = true;
            const LockFreeQueueResult rejected = session.tryPush(input);
            bool threw = false;
            const LockFreeQueueResult pushed = session.push(input);
            LockFreeQueueTestValue::m_throwCopy = false;
            LFQ_CHECK(rejected == LockFreeQueueConstructionFailure && pushed == LockFreeQueueConstructionFailure && session.attached());
            LFQ_CHECK(values.poolStatistics().activeProducers == 1 && values.empty());
            session.push(input);
            LockFreeQueueTestValue::m_throwAssign = true;
            threw = false;
            try
            {
                values.pop(&output);
            }
            catch (const std::runtime_error&)
            {
                threw = true;
            }
            LockFreeQueueTestValue::m_throwAssign = false;
            LFQ_CHECK(threw && values.size() == 1 && values.pop(&output) && output.m_value == 42);
        }
        LFQ_CHECK(values.poolStatistics().activeProducers == 0);
    }
    return LockFreeQueueTestValue::m_live.load() == 0;
}

static std::atomic<bool> lockFreeQueuePaused[8], lockFreeQueueResume[8];

/** 在指定发布完成点暂停单个测试线程，超时记录失败
@param [in] lane 0 至 7 的测试生产者下标
*/
static void LockFreeQueuePausePublisher(size_t lane)
{
    if (lane >= 8)
    {
        LockFreeQueueTestControl::m_timedOut.store(true);
        return;
    }
    lockFreeQueuePaused[lane].store(true);
    const uint64_t deadline = LockFreeQueueTestControl::ticks() + UINT64_C(10000000000);
    while (!lockFreeQueueResume[lane].load() && LockFreeQueueTestControl::ticks() < deadline)
    {
        std::this_thread::yield();
    }
    if (!lockFreeQueueResume[lane].load())
    {
        LockFreeQueueTestControl::m_timedOut.store(true);
    }
}

/** 八个发布者暂停时核对十万次 FIFO 推进与辅助分配上界
@return 数据及分配界限是否全部满足
*/
static bool LockFreeQueueMultiplePausedPublishers()
{
    LockFreeQueue<uint64_t> queue;
    queue.destroy();
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 64;
    config.maximumCapacity = 64;
    config.maximumProducers = 12;
    LFQ_CHECK(queue.configurePool(config) == LockFreeQueueSuccess && queue.tryInit() == LockFreeQueueSuccess);
    LockFreeQueueTestThreads threads;
    LockFreeQueueResult results[8];
    bool valid = true;
    uint64_t output = UINT64_MAX;
    for (size_t p = 0; p < 8; ++p)
    {
        lockFreeQueuePaused[p].store(false);
        lockFreeQueueResume[p].store(false);
        results[p] = LockFreeQueueTemporaryUnavailable;
    }
    for (size_t p = 0; valid && p < 8; ++p)
    {
        threads.m_threads.emplace_back(
            [&, p]()
            {
                g_lockFreeQueueTestLane = static_cast<int32_t>(100 + p);
                results[p] = queue.tryPush(900000 + p);
            });
        const uint64_t deadline = LockFreeQueueTestControl::ticks() + UINT64_C(3000000000);
        while (!lockFreeQueuePaused[p].load() && LockFreeQueueTestControl::ticks() < deadline)
        {
            std::this_thread::yield();
        }
        valid = lockFreeQueuePaused[p].load() && queue.pop(&output) && output == 900000 + p && valid;
        for (uint64_t i = 1; valid && i < 256; ++i)
        {
            valid = queue.tryPush(i) == LockFreeQueueSuccess && queue.pop(&output) && output == i;
        }
    }
    uint64_t allocationsAfterWarmup = 0;
    size_t completed = 0;
    for (; valid && completed < 100000; ++completed)
    {
        valid = queue.tryPush(completed) == LockFreeQueueSuccess && queue.pop(&output) && output == completed;
        if (completed == 49999)
        {
            allocationsAfterWarmup = queue.poolStatistics().allocationCalls;
        }
    }
    const LockFreeQueuePoolStatistics held = queue.poolStatistics();
    for (size_t p = 0; p < 8; ++p)
    {
        lockFreeQueueResume[p].store(true);
    }
    threads.join();
    for (size_t p = 0; p < 8; ++p)
    {
        valid = results[p] == LockFreeQueueSuccess && valid;
    }
    valid = held.currentCapacity == 64 && held.peakReservedCapacity == 64 && held.successfulGrowCount == 0 &&
            held.allocationCalls == allocationsAfterWarmup &&
            held.allocationCalls <= 2 + 4 * config.maximumProducers + 4 && valid;
    queue.maintain(4096);
    LFQ_CHECK(valid && completed == 100000 && queue.empty() && queue.poolStatistics().usedNodes == 1);
    std::cout << "MULTIPLE_PAUSED publishers=8 completed=" << completed << " capacity=" << held.currentCapacity
              << " allocations=" << held.allocationCalls << " allocation_plateau=passed" << std::endl;
    return true;
}

// A deterministic transfer test: the secondary slot is read, then the owner
// transfers protection to the primary slot. The scanner must still see ownership.
/** 受控验证次保护转主保护时扫描不能漏看两者
@return 交错中受保护块是否仍被识别
*/
static bool LockFreeQueueProtectionTransfer()
{
    typedef LockFreeQueueArena<int32_t> Arena;
    Arena pool;
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 2;
    config.maximumCapacity = 2;
    config.maximumProducers = 1;
    LFQ_CHECK(pool.configure(config) == LockFreeQueueSuccess && pool.initialize() == LockFreeQueueSuccess);
    Arena::Record* record = pool.enter();
    Arena::Block* block = pool.initialBlock();
    pool.testProtectTraversal(*record, block);
    LockFreeQueueTestControl::reset();
    LockFreeQueueTestControl::m_pausePoint.store(22);
    bool protectedDuringTransfer = false;
    LockFreeQueueTestThreads threads;
    threads.m_threads.emplace_back(
        [&]()
        {
            g_lockFreeQueueTestLane = 1;
            protectedDuringTransfer = pool.testProtectedBlock(block);
        });
    const bool reached = LockFreeQueueTestControl::waitReached();
    record->m_block.store(block);
    pool.testProtectTraversal(*record, nullptr);
    LockFreeQueueTestControl::m_release.store(true);
    threads.join();
    pool.leave(*record, true);
    LFQ_CHECK(reached && protectedDuringTransfer && !pool.testProtectedBlock(block));
    std::cout << "HAZARD_TRANSFER secondary_to_primary=protected" << std::endl;
    return true;
}
#include "LockFreeQueueArenaTests.inl"

bool LockFreeQueueTests::maintenanceBounds()
{
    LockFreeQueue<int32_t> queue;
    bool valid = configure(queue, 8, 128);
    for (int32_t i = 0; i < 64; ++i)
    {
        queue.push(i);
    }
    for (int32_t i = 0; i < 64; ++i)
    {
        int32_t value = -1;
        valid = queue.pop(&value) && value == i && valid;
    }
    LockFreeQueueTestControl::m_maintenanceVisits.store(0);
    valid = queue.maintain(0).result == LockFreeQueueSuccess && valid;
    valid = LockFreeQueueTestControl::m_maintenanceVisits.load() == 0 && valid;
    valid = queue.maintain(1).result == LockFreeQueueSuccess && valid;
    valid = LockFreeQueueTestControl::m_maintenanceVisits.load() <= 1 && valid;
    LockFreeQueueTestControl::m_maintenanceVisits.store(0);
    valid = queue.maintain(SIZE_MAX).result == LockFreeQueueSuccess && valid;
    const size_t visits = LockFreeQueueTestControl::m_maintenanceVisits.load();
    const LockFreeQueuePoolStatistics stats = queue.poolStatistics();
    valid = visits < 1024 && stats.usedNodes == 1 && stats.retiredNodes == 0 &&
        stats.reusableFreeCapacity + 1 == stats.currentCapacity && valid;
    std::cout << "MAINTENANCE_BOUNDS visits=" << visits << " valid=" << valid << std::endl;
    queue.destroy();
    int32_t untouched = 73;
    valid = !queue.pop(&untouched) && untouched == 73 && valid;
    queue.init();
    for (size_t i = 0; i < 100000; ++i)
    {
        valid = !queue.pop(&untouched) && untouched == 73 && valid;
    }
    queue.push(42);
    return queue.pop(&untouched) && untouched == 42 && valid;
}

bool LockFreeQueueTests::retryContention()
{
    LockFreeQueue<int32_t> queue;
    LockFreeQueueTestControl::m_transientFailures.store(257);
    LFQ_CHECK(queue.tryPush(1) == LockFreeQueueTemporaryUnavailable);
    LFQ_CHECK(LockFreeQueueTestControl::m_transientFailures.load() == 256 && queue.empty());
    LFQ_CHECK(queue.push(2) == LockFreeQueueSuccess);
    LFQ_CHECK(LockFreeQueueTestControl::m_transientFailures.load() == 0);
    int32_t value = 0;
    LFQ_CHECK(queue.pop(&value) && value == 2 && queue.empty());

    LockFreeQueueProducer<int32_t> producer;
    LFQ_CHECK(queue.tryAcquireProducer(producer) == LockFreeQueueSuccess);
    LockFreeQueueTestControl::m_transientFailures.store(1025);
    LFQ_CHECK(producer.tryPush(3) == LockFreeQueueTemporaryUnavailable);
    LFQ_CHECK(LockFreeQueueTestControl::m_transientFailures.load() == 1024 && queue.empty());
    LFQ_CHECK(producer.push(4) == LockFreeQueueSuccess);
    LFQ_CHECK(LockFreeQueueTestControl::m_transientFailures.load() == 0);
    LFQ_CHECK(queue.pop(&value) && value == 4 && queue.empty());
    producer.release();

    // 初始发布段用尽后，完整扫描的 CAS 竞争必须与真实辅助登记满额区分。
    LockFreeQueueArena<int32_t> pool;
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 1;
    config.maximumCapacity = 1;
    config.maximumProducers = 1;
    LFQ_CHECK(pool.configure(config) == LockFreeQueueSuccess && pool.initialize() == LockFreeQueueSuccess);
    auto record = pool.enter();
    LFQ_CHECK(record != nullptr);
    std::vector<LockFreeQueueArena<int32_t>::Segment*> segments;
    for (size_t i = 0; i < 3; ++i)
    {
        auto segment = pool.acquireSegment(*record);
        LFQ_CHECK(segment != nullptr);
        segments.push_back(segment);
    }
    for (size_t mode = 0; mode < 2; ++mode)
    {
        record->m_progress.begin(mode != 0);
        LockFreeQueueTestControl::m_transientFailures.store(8);
        LockFreeQueueResult status = LockFreeQueueSuccess;
        LFQ_CHECK(pool.acquireSegment(*record, false, &status) == nullptr);
        LFQ_CHECK(status == LockFreeQueueTemporaryUnavailable && record->m_progress.interrupted());
        LFQ_CHECK(LockFreeQueueTestControl::m_transientFailures.load() == 0);
        record->m_progress.resume();
        auto recovered = pool.acquireSegment(*record, false, &status);
        LFQ_CHECK(recovered != nullptr && status == LockFreeQueueSuccess);
        pool.releaseSegment(recovered, record);
    }
    for (size_t i = 0; i < 8; ++i)
    {
        auto segment = pool.acquireSegment(*record);
        LFQ_CHECK(segment != nullptr);
        segments.push_back(segment);
    }
    LockFreeQueueResult status = LockFreeQueueSuccess;
    LFQ_CHECK(pool.acquireSegment(*record, false, &status) == nullptr);
    LFQ_CHECK(status == LockFreeQueueCapacityLimit && !record->m_progress.interrupted());
    for (size_t i = 0; i < segments.size(); ++i)
    {
        pool.releaseSegment(segments[i], record);
    }
    pool.leave(*record, true);
    return true;
}

bool LockFreeQueueTests::registrationProgress()
{
    LockFreeQueue<int32_t> queue;
    LFQ_CHECK(configure(queue, 256, 256, 0, 1));
    LockFreeQueueProducer<int32_t> owner;
    LFQ_CHECK(queue.tryAcquireProducer(owner) == LockFreeQueueSuccess);
    for (int32_t index = 0; index < 64; ++index)
    {
        owner.push(index);
        int32_t value = -1;
        LFQ_CHECK(queue.pop(&value) && value == index);
    }
    LFQ_CHECK(queue.poolStatistics().reusableFreeCapacity != 0);
    std::atomic<bool> finished(false);
    std::atomic<int32_t> outcome(-1);
    std::thread caller([&]() {
        try
        {
            outcome.store(queue.push(99));
        }
        catch (const LockFreeQueueError&)
        {
            outcome.store(-3);
        }
        catch (...)
        {
            outcome.store(-2);
        }
        finished.store(true);
    });
    const std::chrono::steady_clock::time_point deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!finished.load() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::yield();
    }
    const bool returnedBeforeRelease = finished.load();
    // 无论新旧实现结果如何，都解除基线的等待并正常 join，不用 abort/assert 杀进程。
    owner.release();
    caller.join();
    std::cout << "REGISTRATION_PROGRESS returned_before_release=" << returnedBeforeRelease
              << " result=" << outcome.load() << std::endl;
    LFQ_CHECK(returnedBeforeRelease && outcome.load() == LockFreeQueueCapacityLimit);
    LFQ_CHECK(queue.empty());
    queue.push(100);
    int32_t value = -1;
    return queue.pop(&value) && value == 100 && queue.empty();
}

bool LockFreeQueueTests::cacheResourceProgress()
{
    LockFreeQueue<int32_t> queue;
    LFQ_CHECK(configure(queue, 128, 128, 0, 2));
    LockFreeQueueProducer<int32_t> owner, writer;
    LFQ_CHECK(queue.tryAcquireProducer(owner) == LockFreeQueueSuccess);
    LFQ_CHECK(queue.tryAcquireProducer(writer) == LockFreeQueueSuccess);
    owner.push(0);
    for (int32_t index = 1; index <= 111; ++index)
    {
        writer.push(index);
    }
    LFQ_CHECK(writer.tryPush(112) == LockFreeQueueCapacityLimit);
    std::atomic<bool> finished(false);
    std::atomic<int32_t> outcome(-1);
    std::thread caller([&]() {
        try
        {
            outcome.store(writer.push(112));
        }
        catch (const LockFreeQueueError&)
        {
            outcome.store(-3);
        }
        catch (...)
        {
            outcome.store(-2);
        }
        finished.store(true);
    });
    const std::chrono::steady_clock::time_point deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (!finished.load() && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::yield();
    }
    const bool returnedBeforeRelease = finished.load();
    owner.release();
    caller.join();
    writer.release();
    std::cout << "CACHE_RESOURCE_PROGRESS returned_before_release=" << returnedBeforeRelease
              << " result=" << outcome.load() << std::endl;
    LFQ_CHECK(returnedBeforeRelease && outcome.load() == LockFreeQueueCapacityLimit);
    for (int32_t index = 0; index <= 111; ++index)
    {
        int32_t value = -1;
        LFQ_CHECK(queue.pop(&value) && value == index);
    }
    LFQ_CHECK(queue.empty());
    queue.push(112);
    int32_t value = -1;
    return queue.pop(&value) && value == 112 && queue.empty();
}

bool LockFreeQueueTests::busyContract()
{
    LFQ_CHECK(LockFreeQueueBusy == LockFreeQueueTemporaryUnavailable && LockFreeQueueBusy == 5);
    LockFreeQueueArena<int32_t> uninitialized;
    for (size_t mode = 0; mode < 2; ++mode)
    {
        LockFreeQueueResult status = LockFreeQueueSuccess;
        LFQ_CHECK(uninitialized.enter(mode != 0, &status) == nullptr && status == LockFreeQueueNotInitialized);
    }
    LockFreeQueue<int32_t> queue(false);
    LockFreeQueuePoolConfig config;
    config.initialCapacity = 64;
    config.maximumCapacity = 64;
    config.maximumProducers = 1;
    LFQ_CHECK(queue.configurePool(config) == LockFreeQueueSuccess && queue.tryInit() == LockFreeQueueSuccess);
    LFQ_CHECK(queue.configurePool(config) == LockFreeQueueInvalidState);
    LFQ_CHECK(queue.configureEntryOnly(true) == LockFreeQueueInvalidState);
    LFQ_CHECK(queue.configureProducerCounters(true) == LockFreeQueueInvalidState);
    LFQ_CHECK(queue.configureCancelableReservations(true) == LockFreeQueueInvalidState);

    // 故意令真实登记 CAS 的 expected 不匹配；节点充足，不模拟容量不足。
    LockFreeQueueProducer<int32_t> producer, extra;
    LockFreeQueueTestControl::m_failPoint.store(31);
    LockFreeQueueTestControl::m_transientFailures.store(1);
    LFQ_CHECK(queue.tryAcquireProducer(producer) == LockFreeQueueBusy && !producer.attached());
    LockFreeQueueTestControl::m_failPoint.store(0);
    LFQ_CHECK(queue.tryAcquireProducer(producer) == LockFreeQueueSuccess);
    LFQ_CHECK(queue.tryAcquireProducer(producer) == LockFreeQueueInvalidState);
    LFQ_CHECK(queue.tryAcquireProducer(extra) == LockFreeQueueCapacityLimit && !extra.attached());
    LFQ_CHECK(queue.tryPush(1) == LockFreeQueueCapacityLimit);
    LFQ_CHECK(queue.push(1) == LockFreeQueueCapacityLimit);
    LFQ_CHECK(queue.empty() && queue.poolStatistics().reusableFreeCapacity == 63);
    producer.release();

    for (int32_t point = 31; point <= 32; ++point)
    {
        LFQ_CHECK(configure(queue, 64, 64, 0, 1));
        LockFreeQueueTestControl::m_failPoint.store(point);
        LockFreeQueueTestControl::m_transientFailures.store(1);
        LFQ_CHECK(queue.tryPush(1) == LockFreeQueueBusy && queue.empty());
        LockFreeQueueTestControl::m_transientFailures.store(1025);
        LFQ_CHECK(queue.push(2) == LockFreeQueueSuccess);
        LFQ_CHECK(LockFreeQueueTestControl::m_transientFailures.load() == 0);
        LockFreeQueueTestControl::m_failPoint.store(0);
        int32_t value = -1;
        LFQ_CHECK(queue.pop(&value) && value == 2 && queue.empty());
    }

    LFQ_CHECK(configure(queue, 64, 64, 0, 1));
    LFQ_CHECK(queue.tryAcquireProducer(producer) == LockFreeQueueSuccess);
    LockFreeQueueTestControl::m_failPoint.store(32);
    LockFreeQueueTestControl::m_transientFailures.store(1);
    LFQ_CHECK(producer.tryPush(3) == LockFreeQueueBusy && queue.empty());
    LockFreeQueueTestControl::m_transientFailures.store(1025);
    LFQ_CHECK(producer.push(4) == LockFreeQueueSuccess);
    LFQ_CHECK(LockFreeQueueTestControl::m_transientFailures.load() == 0);
    LockFreeQueueTestControl::m_failPoint.store(0);
    int32_t value = -1;
    LFQ_CHECK(queue.pop(&value) && value == 4 && queue.empty());
    producer.release();

    // Fence 和外部 Entry 的 try 包装必须保留真实 CAS 失败，不伪装满池。
    for (size_t mode = 0; mode < 8; ++mode)
    {
        LFQ_CHECK(configure(queue, 64, 64, 0, 1));
        const bool attached = mode == 2 || mode == 3 || mode >= 6;
        if (attached)
        {
            LFQ_CHECK(queue.tryAcquireProducer(producer) == LockFreeQueueSuccess);
        }
        LockFreeQueueEntry<int32_t> entry(6);
        auto submit = [&]() -> LockFreeQueueResult {
            switch (mode)
            {
            case 0: return queue.tryPush(6);
            case 1: return queue.tryPushAndFence(6);
            case 2: return producer.tryPush(6);
            case 3: return producer.tryPushAndFence(6);
            case 4: return queue.tryPushEntry(entry);
            case 5: return queue.tryPushEntryAndFence(entry);
            case 6: return producer.tryPushEntry(entry);
            case 7: return producer.tryPushEntryAndFence(entry);
            default: return LockFreeQueueInvalidConfiguration;
            }
        };
        LockFreeQueueTestControl::m_failPoint.store(32);
        LockFreeQueueTestControl::m_transientFailures.store(1);
        LFQ_CHECK(submit() == LockFreeQueueBusy && queue.empty());
        LFQ_CHECK(LockFreeQueueTestControl::m_transientFailures.load() == 0);
        LFQ_CHECK(entry.m_cookie == 0 && entry.m_owner == nullptr);
        LockFreeQueueTestControl::m_failPoint.store(0);
        LFQ_CHECK(submit() == LockFreeQueueSuccess);
        LFQ_CHECK(queue.pop(&value) && value == 6 && queue.empty());
        producer.release();
    }

    // 所有入队形式均检查真正的满池拒绝、无提交副作用及消费后的恢复。
    for (size_t mode = 0; mode < 10; ++mode)
    {
        LFQ_CHECK(configure(queue, 2, 2, 0, 2));
        const bool attached = (mode >= 3 && mode <= 5) || mode >= 8;
        if (attached)
        {
            LFQ_CHECK(queue.tryAcquireProducer(producer) == LockFreeQueueSuccess);
        }
        LFQ_CHECK((attached ? producer.push(7) : queue.push(7)) == LockFreeQueueSuccess);
        LockFreeQueueEntry<int32_t> entry(8);
        auto submit = [&]() -> LockFreeQueueResult {
            switch (mode)
            {
            case 0: return queue.push(8);
            case 1: return queue.tryPush(8);
            case 2: return queue.tryPushAndFence(8);
            case 3: return producer.push(8);
            case 4: return producer.tryPush(8);
            case 5: return producer.tryPushAndFence(8);
            case 6: return queue.tryPushEntry(entry);
            case 7: return queue.tryPushEntryAndFence(entry);
            case 8: return producer.tryPushEntry(entry);
            case 9: return producer.tryPushEntryAndFence(entry);
            default: return LockFreeQueueInvalidConfiguration;
            }
        };
        size_t policyCalls = 0;
        LFQ_CHECK(queue.setGrowPolicy([&](const LockFreeQueueGrowContext&) -> size_t {
            ++policyCalls;
            if ((mode & 1) != 0) { throw std::runtime_error("full pool policy must not run"); }
            return 0;
        }) == LockFreeQueueSuccess);
        LFQ_CHECK(submit() == LockFreeQueueCapacityLimit && queue.size() == 1 && policyCalls == 0);
        LFQ_CHECK(queue.pop(&value) && value == 7 && queue.empty());
        LFQ_CHECK(submit() == LockFreeQueueSuccess);
        LFQ_CHECK(queue.pop(&value) && value == 8 && queue.empty());
        LFQ_CHECK(queue.setGrowPolicy(LockFreeQueueGrowPolicy()) == LockFreeQueueSuccess);
        producer.release();
    }
    std::cout << "BUSY_CONTRACT cas_sites=2 retries=1025 try_variants=8 full_variants=10 valid=1" << std::endl;
    return true;
}


bool LockFreeQueueTests::cooperativeProgress()
{
    LockFreeQueueProgress progress;
    progress.begin(true);
    for (size_t i = 0; i < 4095; ++i)
    {
        progress.advance();
    }
    LFQ_CHECK(LockFreeQueueTestControl::m_scheduleYields.load() == 0);
    progress.advance();
    LFQ_CHECK(LockFreeQueueTestControl::m_scheduleYields.load() == 1);
    for (size_t i = 0; i < 1025; ++i)
    {
        LFQ_CHECK(progress.retry());
    }
#if !defined(__EMSCRIPTEN__) || defined(__EMSCRIPTEN_PTHREADS__)
    LFQ_CHECK(LockFreeQueueTestControl::m_scheduleSleeps.load() == 4);
#else
    LFQ_CHECK(LockFreeQueueTestControl::m_scheduleSleeps.load() == 0);
#endif
    LFQ_CHECK(LockFreeQueueTestControl::m_scheduleYields.load() >= 61 && !progress.interrupted());
    progress.begin(false);
    LFQ_CHECK(!progress.retry() && progress.interrupted());
    progress.resume();
    LFQ_CHECK(!progress.interrupted());
    progress.begin(false);
    LFQ_CHECK(progress.retry(2) && progress.retry(2) && !progress.retry(2) && progress.interrupted());

    LockFreeQueue<int32_t> queue;
    LFQ_CHECK(configure(queue, 8193, 8193, 0, 4));
    LockFreeQueueTestControl::m_scheduleYields.store(0);
    for (int32_t i = 0; i < 4096; ++i)
    {
        LFQ_CHECK(queue.push(i) == LockFreeQueueSuccess);
    }
    const size_t producerYields = LockFreeQueueTestControl::m_scheduleYields.load();
    LFQ_CHECK(producerYields != 0);
    LockFreeQueueTestControl::m_scheduleYields.store(0);
    for (int32_t i = 0; i < 4096; ++i)
    {
        int32_t value = -1;
        LFQ_CHECK(queue.pop(&value) && value == i);
    }
    const size_t consumerYields = LockFreeQueueTestControl::m_scheduleYields.load();
    LFQ_CHECK(consumerYields != 0);
    LockFreeQueueTestControl::m_scheduleYields.store(0);
    for (size_t i = 0; i < 16384; ++i)
    {
        LFQ_CHECK(!queue.pop(nullptr));
    }
    const size_t emptyYields = LockFreeQueueTestControl::m_scheduleYields.load();
    LFQ_CHECK(emptyYields >= 4);
    LFQ_CHECK(configure(queue, 1, 8, 0, 4));
    LockFreeQueueTestControl::m_failPoint.store(11);
    const LockFreeQueueResult allocation = queue.push(7);
    LockFreeQueueTestControl::m_failPoint.store(0);
    LFQ_CHECK(allocation == LockFreeQueueAllocationFailure && queue.empty());
    LFQ_CHECK(queue.push(7) == LockFreeQueueSuccess && queue.pop(nullptr));
    typedef LockFreeQueueArenaRecord<int32_t> ProgressRecord;
    LFQ_CHECK(sizeof(ProgressRecord) == (sizeof(void*) == 4 ? 64 : 128));
    LFQ_CHECK(offsetof(ProgressRecord, m_progress) + sizeof(LockFreeQueueProgress) <= 32);
    std::cout << "COOPERATIVE_PROGRESS producer_yields=" << producerYields
              << " consumer_yields=" << consumerYields << " empty_yields=" << emptyYields
              << " record_bytes=" << sizeof(LockFreeQueueArenaRecord<int32_t>) << std::endl;
    return true;
}

bool LockFreeQueueTests::dispatch(int32_t id)
{
    switch (id)
    {
    case 1:
        return atomicValues() && fifo();
    case 2:
        return mpsc(4, 10000, false);
    case 3:
        return lifecycle();
    case 4:
        return mpsc(1, 20000, true) && mpsc(2, 20000, true) && mpsc(4, 20000, true) && mpsc(8, 20000, true);
    case 6:
        return ownership();
    case 7:
        return concurrentClear();
    case 8:
        return reuse();
    case 10:
        return alignment() && LockFreeQueueArenaLazyLifetimes();
    case 11:
        return exceptions();
    case 12:
        return boundaries();
    case 13:
        return thresholds();
    case 14:
        return LockFreeQueueArenaPolicies() && policies();
    case 15:
        return allocations() && LockFreeQueueArenaAuxiliaryBound();
    case 16:
        return publication(2);
    case 17:
        return publication(3);
    case 18:
        return LockFreeQueueMultiplePausedPublishers() && initialBatches() && steady(1, false, true) &&
               steady(1, true, true);
    case 19:
        return LockFreeQueueGrowthPublicationRace() && reservations();
    case 20:
        return LockFreeQueueArenaCapacities(0) && LockFreeQueueArenaCapacities(8) && shrink();
    case 21:
        return LockFreeQueueProtectionTransfer() && poolInterleavings() && pausedConsumer() &&
               LockFreeQueueArenaConcurrentClaims();
    case 25:
        return steady(1);
    case 26:
        return cacheReuse();
    case 27:
        return cacheInterleavings();
    case 28:
        return counters();
    case 29:
        return publicationOrder();
    case 30:
        return segmentReuse();
    case 31:
        return cachedBatches();
    case 32:
        return capacityContracts();
    case 34:
        return reservedProducer();
    case 36:
        return steady(1, true);
    case 37:
        return growthTransitions() && steady(1, false, false, true) && steady(1, true, false, true);
    case 39:
        return maintenanceBounds();
    case 41:
        return retryContention();
    case 42:
        return registrationProgress();
    case 43:
        return cacheResourceProgress();
    case 44:
        return cooperativeProgress();
    case 45:
        return busyContract();
    default:
        return false;
    }
}

int LockFreeQueueTests::run(int argc, char** argv)
{
    const std::map<int32_t, std::string> names = {{1, "单生产者FIFO"}, {2, "多生产者集合及顺序"}, {3, "生命周期"},
        {4, "分级并发压力"}, {6, "载荷所有权"}, {7, "清理并发"}, {8, "池复用"}, {10, "非默认构造"},
        {11, "复制赋值异常"}, {12, "空及非法配置"}, {13, "阈值与上限"}, {14, "策略与饱和"}, {15, "分配故障"},
        {16, "发布前暂停"}, {17, "发布后暂停"}, {18, "暂停发布段保护"}, {19, "扩容预留并发"}, {20, "动态多批缩容"},
        {21, "池保护及缩容竞争"}, {25, "持续有界压力"}, {26, "缓存上限及共享组压力"}, {27, "缓存转交和小预算维护"},
        {28, "模计数及回绕观察"}, {29, "恢复生产者后的发布顺序"}, {30, "发布段复用及序号耗尽"},
        {31, "批量空闲缓存及接手"}, {32, "固定池载荷及策略切换"}, {34, "生产者上下文及租约转交"},
        {36, "上下文持续有界压力"}, {37, "初始块及动态增长"}, {39, "空返回及维护工作上限"}, {41, "普通入队持续重试竞争"},
        {42, "登记槽占满时明确结束"}, {43, "活跃缓存占额时明确结束"}, {44, "生产消费调度让步及错误码"}, {45, "Busy竞争与空间不足分离"}};
    if ((argc == 3 || argc == 4) && (std::string(argv[1]) == "25" || std::string(argv[1]) == "36"))
    {
        const std::string seconds(argv[2]);
        if (seconds.empty() || seconds.find_first_not_of("0123456789") != std::string::npos)
        {
            return 2;
        }
        const long value = std::strtol(seconds.c_str(), nullptr, 10);
        if (value <= 0 || value > 86400)
        {
            return 2;
        }
        if (argc == 4 && std::string(argv[3]) != "dynamic" && std::string(argv[3]) != "prefix")
        {
            return 2;
        }
        return steady(static_cast<uint32_t>(value), std::string(argv[1]) == "36",
                   argc == 4 && std::string(argv[3]) == "dynamic", argc == 4 && std::string(argv[3]) == "prefix")
                   ? 0
                   : 1;
    }
    if (argc != 2)
    {
        std::cout << "Usage: LockFreeQueueTests <all|id,id,...>" << std::endl;
        return 2;
    }
    std::set<int32_t> selected;
    const std::string input(argv[1]);
    if (input == "all")
    {
        for (std::map<int32_t, std::string>::const_iterator it = names.begin(); it != names.end(); ++it)
        {
            selected.insert(it->first);
        }
    }
    else
    {
        std::istringstream parser(input);
        std::string token;
        while (std::getline(parser, token, ','))
        {
            if (token.empty() || token.find_first_not_of("0123456789") != std::string::npos)
            {
                return 2;
            }
            const long number = std::strtol(token.c_str(), nullptr, 10);
            if (number <= 0 || number > INT32_MAX || names.count(static_cast<int32_t>(number)) == 0)
            {
                return 2;
            }
            selected.insert(static_cast<int32_t>(number));
        }
    }
    if (selected.empty() || (!input.empty() && input[input.size() - 1] == ','))
    {
        return 2;
    }
    size_t failures = 0;
    std::cout << "ENV pointer_bytes=" << sizeof(void*) << " cpp=" << __cplusplus;
#ifdef _MSC_VER
    std::cout << " msc=" << _MSC_VER;
#endif
#ifdef _DEBUG
    std::cout << " config=Debug";
#else
    std::cout << " config=Release";
#endif
    std::cout << " seed=20261104" << std::endl;
    for (std::set<int32_t>::const_iterator it = selected.begin(); it != selected.end(); ++it)
    {
        LockFreeQueueTestControl::reset();
        const uint64_t start = LockFreeQueueTestControl::ticks();
        bool passed = false;
        std::cout << "BEGIN id=" << *it << " name=" << names.find(*it)->second << std::endl;
        try
        {
            passed = dispatch(*it);
        }
        catch (const std::exception& error)
        {
            std::cout << "EXCEPTION " << error.what() << std::endl;
        }
        catch (...)
        {
            std::cout << "EXCEPTION unknown" << std::endl;
        }
        passed = passed && !LockFreeQueueTestControl::m_timedOut.load();
        failures += passed ? 0 : 1;
        std::cout << (passed ? "PASS" : "FAIL") << " id=" << *it
                  << " elapsed_ns=" << LockFreeQueueTestControl::ticks() - start << std::endl;
    }
    std::cout << "SUMMARY executed=" << selected.size() << " failed=" << failures << std::endl;
    return failures == 0 ? 0 : 1;
}

int main(int argc, char** argv)
{
    try
    {
        return LockFreeQueueTests::run(argc, argv);
    }
    catch (const std::exception& error)
    {
        std::cerr << "HARNESS_FAIL " << error.what() << std::endl;
        return 2;
    }
}