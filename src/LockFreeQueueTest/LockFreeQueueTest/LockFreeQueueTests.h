#pragma once

#include "LockFreeQueueInterfaceContract.h"
#include <memory>
#include <thread>
#include <vector>
#include <map>
#include <string>

// Stable registry; external IDs are dispatched by the C++ LockFreeQueueTestTool runner.
enum LockFreeQueueTestId
{
    LockFreeQueueTestFifo = 1,
    LockFreeQueueTestMpsc = 2,
    LockFreeQueueTestLifecycle = 3,
    LockFreeQueueTestScale = 4,
    LockFreeQueueTestPerformance = 5,
    LockFreeQueueTestOwnership = 6,
    LockFreeQueueTestClear = 7,
    LockFreeQueueTestReuse = 8,
    LockFreeQueueTestNotification = 9,
    LockFreeQueueTestAlignment = 10,
    LockFreeQueueTestExceptions = 11,
    LockFreeQueueTestBoundaries = 12,
    LockFreeQueueTestThresholds = 13,
    LockFreeQueueTestPolicies = 14,
    LockFreeQueueTestAllocation = 15,
    LockFreeQueueTestBeforePublish = 16,
    LockFreeQueueTestAfterPublish = 17,
    LockFreeQueueTestProtectedNode = 18,
    LockFreeQueueTestReservation = 19,
    LockFreeQueueTestShrink = 20,
    LockFreeQueueTestInterleavings = 21,
    LockFreeQueueTestBacklog = 22,
    LockFreeQueueTestFourHeaders = 23,
    LockFreeQueueTestOldDefect = 24,
    LockFreeQueueTestSteady = 25,
    LockFreeQueueTestCache = 26,
    LockFreeQueueTestCachePause = 27,
    LockFreeQueueTestCounters = 28,
    LockFreeQueueTestPublicationOrder = 29,
    // Fixed ring generations, reuse and structural exhaustion.
    LockFreeQueueTestRingReuse = 30,
    // Bounded private batches and stealing unused cached nodes.
    LockFreeQueueTestRingCachedBatches = 31,
    // Payload contracts and policy continuity across capacity modes.
    LockFreeQueueTestRingContracts = 32,
    // Producer-only throughput; drain and independent oracle outside timing.
    LockFreeQueueTestProducerPerformance = 33,
    // Pool growth/reuse, shrink-regrow and bounded-backpressure measurements.
    LockFreeQueueTestPoolPerformance = 38
};

// Test-only control, never part of the production ABI.
class LockFreeQueueTestControl
{
public:
    static std::atomic<int32_t> m_pausePoint;
    static std::atomic<int32_t> m_failPoint;
    static std::atomic<bool> m_reached;
    static std::atomic<bool> m_release;
    static std::atomic<bool> m_timedOut;
    static std::atomic<uint64_t> m_clock;
    static std::atomic<size_t> m_reclaimVisits;
    static std::atomic<size_t> m_maintenanceVisits;
    // 仅隔离白盒副本消费的未提交失败次数
    static std::atomic<int32_t> m_transientFailures;
    static std::atomic<size_t> m_scheduleYields;
    static std::atomic<size_t> m_scheduleSleeps;

    /** Reset an isolated test's hooks */
    static void reset();

    /** Wait for a hook within its test deadline */
    static bool waitReached();

    /** Return monotonic elapsed nanoseconds */
    static uint64_t ticks();
    /** Read this process's handle count; zero on non-Windows hosts */
    static uint32_t handles();
    /** Read process private bytes; zero when the host has no probe */
    static uint64_t privateBytes();
    /** Observe the same handle count for 1.5 seconds, deadline 5 seconds */
    static uint32_t settledHandles();
};

#ifdef _MSC_VER
class __declspec(align(64)) LockFreeQueueTestAligned
#else
class alignas(64) LockFreeQueueTestAligned
#endif
{
public:
    int32_t m_value;
    static bool m_valid;
    /** No default construction; check every constructed payload address */
    explicit LockFreeQueueTestAligned(int32_t value);
    /** Validate the pool's real placement address */
    LockFreeQueueTestAligned(const LockFreeQueueTestAligned& other);
    /** Copy out a value */
    LockFreeQueueTestAligned& operator=(const LockFreeQueueTestAligned& other);
};

// A deliberately non-default-constructible throwing payload.
class LockFreeQueueTestValue
{
public:
    int32_t m_value;
    static std::atomic<int32_t> m_live;
    static bool m_throwCopy;
    static bool m_throwAssign;

    /** Construct a counted payload */
    explicit LockFreeQueueTestValue(int32_t value);

    /** Copy, optionally injecting failure before object lifetime begins */
    LockFreeQueueTestValue(const LockFreeQueueTestValue& other);

    /** Modify output then optionally throw; never modify the source */
    LockFreeQueueTestValue& operator=(const LockFreeQueueTestValue& other);

    /** Release one counted lifetime */
    ~LockFreeQueueTestValue();
};

#if defined(_MSC_VER) && _MSC_VER < 1900
#define LOCK_FREE_QUEUE_TEST_NOTHROW throw()
#else
#define LOCK_FREE_QUEUE_TEST_NOTHROW noexcept
#endif

class LockFreeQueueTestMove
{
public:
    int32_t m_value;
    /** Construct the value used by the independent move oracle */
    explicit LockFreeQueueTestMove(int32_t value);
    /** Push must still copy */
    LockFreeQueueTestMove(const LockFreeQueueTestMove& other);
    /** This throws, detecting an unnecessary copy in the nonthrowing-move path */
    LockFreeQueueTestMove& operator=(const LockFreeQueueTestMove& other);
    /** Transfer and poison the source without throwing */
    LockFreeQueueTestMove& operator=(LockFreeQueueTestMove&& other) LOCK_FREE_QUEUE_TEST_NOTHROW;
};

class LockFreeQueueTestThrowMove
{
public:
    int32_t m_value;
    /** Construct a value whose move must never be chosen */
    explicit LockFreeQueueTestThrowMove(int32_t value);
    /** Copy on push */
    LockFreeQueueTestThrowMove(const LockFreeQueueTestThrowMove& other);
    /** Safe fallback preserving the queued source */
    LockFreeQueueTestThrowMove& operator=(const LockFreeQueueTestThrowMove& other);
    /** Poison the source and throw if the implementation chooses incorrectly */
    LockFreeQueueTestThrowMove& operator=(LockFreeQueueTestThrowMove&& other);
};

// All test threads are joined even if checks/allocations throw.
class LockFreeQueueTestThreads
{
public:
    std::vector<std::thread> m_threads;

    /** Join any remaining threads */
    ~LockFreeQueueTestThreads();

    /** Join each started thread once */
    void join();
};

// Callable whose copy can fail while configuring std::function.
class LockFreeQueueTestPolicy
{
public:
    static bool m_throwCopy;

    /** Construct a test policy */
    LockFreeQueueTestPolicy();

    /** Copy with a controlled exception */
    LockFreeQueueTestPolicy(const LockFreeQueueTestPolicy& other);

    /** Request two extra nodes */
    size_t operator()(const LockFreeQueueGrowContext& context) const;
};

// Stable IDs and independent test implementations, shared by all CLI selections.
class LockFreeQueueTests
{
public:
    /** Run selected cases and emit machine-readable PASS/FAIL lines */
    static int run(int argc, char** argv);

    /** Single producer, discard and complete FIFO oracle */
    static bool fifo();
    /** Check actual generic atomic width, values and failed-CAS expected updates */
    static bool atomicValues();
    /** A paused initial-batch owner cannot stop other producers or exceed capacity */
    static bool initialBatches();
    /** Initial and grown block publication boundaries and complete physical-node reuse */
    static bool growthTransitions();

    /** Large maintenance budgets stop after useful scans; zero/one preserve bounds */
    static bool maintenanceBounds();

    /** 普通push及生产者上下文必须跨过任意次数未提交竞争，try入口仍一次尝试
    @return 接纳次数、顺序和剩余注入次数均符合预期
    */
    static bool retryContention();

    /** 登记槽被长期上下文占用时，普通入队必须明确失败而不等待持有者 */
    static bool registrationProgress();

    /** 私有缓存占用剩余额度时，绑定生产者明确失败而不等待另一上下文释放 */
    static bool cacheResourceProgress();

    /** 核对竞争退避、成功路径/空轮询调度让步及普通入队的资源错误码 */
    static bool cooperativeProgress();

    /** Busy 仅表示 try 竞争；真实满额和状态错误必须独立，失败后可恢复 */
    static bool busyContract();

    /** Multiple producers: independent sequence/ID/accepted-set oracle */
    static bool mpsc(size_t producers, size_t count, bool bounded);

    /** Verify the bounded cache and concurrent group sharing */
    static bool cacheReuse();

    /** Pause a cache ownership transfer and exercise one-visit maintenance */
    static bool cacheInterleavings();

    /** Check modular counts, leased-record handoff and concurrent observers */
    static bool counters();

    /** Repeat lifecycle transitions and independent instances */
    static bool lifecycle();

    /** Immediate shared lifetime release and caller-owned raw pointers */
    static bool ownership();

    /** Legal clear/push concurrency and payload cleanup */
    static bool concurrentClear();

    /** Repeated reuse without new system allocations */
    static bool reuse();

    /** Non-default construction and copy/assignment failures */
    static bool exceptions();

    /** Verify 64-byte aligned, non-default-constructible inline storage */
    static bool alignment();

    /** Invalid config, max=1 and uninitialized API results */
    static bool boundaries();

    /** Threshold 0/positive, cap and speculative failure */
    static bool thresholds();

    /** Policy return values/context/copy failure/saturation */
    static bool policies();

    /** Each real pool allocation point, rollback and retry */
    static bool allocations();

    /** Pause one producer at a publication boundary */
    static bool publication(int32_t point);

    /** Resume the paused producer before consuming; preserve actual publication order */
    static bool publicationOrder();

    /** Paused segment publishers, repeated reuse and structural exhaustion */
    static bool segmentReuse();

    /** Bounded cached batches, paused ownership and complete reuse at the limit */
    static bool cachedBatches();

    /** Payload contracts and canonical policy state across capacity modes */
    static bool capacityContracts();
    /** Bound producer lease reuse, explicit release and synchronized handoff */
    static bool reservedProducer();

    /** Concurrent reservation collisions and post-publication pause */
    static bool reservations();

    /** Dynamic idle time, multiple blocks, minimum, regrowth */
    static bool shrink();

    /** Pool traversal/closing/reclamation interleavings */
    static bool poolInterleavings();

    /** Pause the sole consumer in copy/reclamation/block closure */
    static bool pausedConsumer();
    /** One bounded queue throughout a timed window; exact per-lane accepted sets */
    static bool steady(uint32_t seconds, bool producerSession = false, bool dynamicPool = false, bool prefixPool = false);

    /** Set a small isolated pool; lifecycle is quiescent */
    static bool configure(LockFreeQueue<int32_t>& queue, size_t initial, int64_t maximum,
        size_t threshold = 0, size_t producers = 16);

    /** Map registry ID to a real test implementation */
    static bool dispatch(int32_t id);
};