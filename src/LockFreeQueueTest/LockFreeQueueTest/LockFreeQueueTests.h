#pragma once

#include "LockFreeQueueInterfaceContract.h"
#include <memory>
#include <thread>
#include <vector>
#include <map>
#include <string>
#include <cstdint>

// Test-only TLS lane: zero bypasses pause hooks; one permits the selected pause.
#ifdef _MSC_VER
extern __declspec(thread) int32_t g_lockFreeQueueTestLane;
#else
extern thread_local int32_t g_lockFreeQueueTestLane;
#endif

// Stable registry; external IDs are dispatched by the C++ LockFreeQueueTestTool runner.
enum LockFreeQueueTestId
{
    // 单生产者及丢弃输出的FIFO
    LockFreeQueueTestFifo = 1,
    // 多生产者集合与每线程顺序
    LockFreeQueueTestMpsc = 2,
    // 初始化销毁和多实例隔离
    LockFreeQueueTestLifecycle = 3,
    // 有限小池分级并发压力
    LockFreeQueueTestScale = 4,
    // 通用吞吐及载荷成本
    LockFreeQueueTestPerformance = 5,
    // 共享载荷及裸指针所有权
    LockFreeQueueTestOwnership = 6,
    // 唯一消费者clear与生产并发
    LockFreeQueueTestClear = 7,
    // 排空复用无新增分配
    LockFreeQueueTestReuse = 8,
    // 真实通知组件接入冒烟
    LockFreeQueueTestNotification = 9,
    // 非默认构造及64字节对齐
    LockFreeQueueTestAlignment = 10,
    // 复制和赋值异常回滚
    LockFreeQueueTestExceptions = 11,
    // 非法容量和未初始化结果
    LockFreeQueueTestBoundaries = 12,
    // 增长阈值与物理上限
    LockFreeQueueTestThresholds = 13,
    // 策略上下文及饱和计数
    LockFreeQueueTestPolicies = 14,
    // 各分配点失败后的恢复
    LockFreeQueueTestAllocation = 15,
    // 发布前暂停不阻塞其它线程
    LockFreeQueueTestBeforePublish = 16,
    // 发布后暂停不依赖补链
    LockFreeQueueTestAfterPublish = 17,
    // 受保护发布段及初始批次
    LockFreeQueueTestProtectedNode = 18,
    // 并发扩容预留和发布竞争
    LockFreeQueueTestReservation = 19,
    // 空闲时长及物理缩容下限
    LockFreeQueueTestShrink = 20,
    // 池保护转交与回收交错
    LockFreeQueueTestInterleavings = 21,
    // 不同积压量的逐项消费
    LockFreeQueueTestBacklog = 22,
    // 四库头文件同编译单元
    LockFreeQueueTestFourHeaders = 23,
    // 历史补链缺陷的独立复现
    LockFreeQueueTestOldDefect = 24,
    // 持续有界多生产压力
    LockFreeQueueTestSteady = 25,
    // 私有空闲批次及共享组
    LockFreeQueueTestCache = 26,
    // 缓存转交与小预算维护
    LockFreeQueueTestCachePause = 27,
    // 模计数和回绕观察
    LockFreeQueueTestCounters = 28,
    // 恢复发布者后的实际发布顺序
    LockFreeQueueTestPublicationOrder = 29,
    // Fixed ring generations, reuse and structural exhaustion.
    LockFreeQueueTestRingReuse = 30,
    // Bounded private batches and stealing unused cached nodes.
    LockFreeQueueTestRingCachedBatches = 31,
    // Payload contracts and policy continuity across capacity modes.
    LockFreeQueueTestRingContracts = 32,
    // Producer-only throughput; drain and independent oracle outside timing.
    LockFreeQueueTestProducerPerformance = 33,
    // Bound producer lifetime and serial handoff
    LockFreeQueueTestProducerLease = 34,
    // Sufficient-capacity producer-only and simultaneous throughput
    LockFreeQueueTestRegisteredPerformance = 35,
    // Bound producer sustained finite-capacity pressure
    LockFreeQueueTestBoundSteady = 36,
    // Initial block and dynamic growth transitions
    LockFreeQueueTestGrowthTransitions = 37,
    // Pool growth/reuse, shrink-regrow and bounded-backpressure measurements
    LockFreeQueueTestPoolPerformance = 38,
    // Bounded maintenance visits, including zero and very large budgets
    LockFreeQueueTestMaintenanceBounds = 39,
    // 750 initial nodes with 1000/500 backlog waves
    LockFreeQueueTestWavePerformance = 40,
    // Ordinary enqueue retries arbitrary uncommitted contention
    LockFreeQueueTestRetryContention = 41,
    // Exhausted producer records return a resource error
    LockFreeQueueTestRegistrationProgress = 42,
    // Cached physical capacity cannot force an indefinite retry
    LockFreeQueueTestCacheResourceProgress = 43,
    // Producer/consumer bounded cooperative scheduling
    LockFreeQueueTestCooperativeProgress = 44,
    // Busy is distinct from physical capacity and state errors
    LockFreeQueueTestBusyContract = 45,
    // Precreated shared_ptr values, separate and simultaneous phases
    LockFreeQueueTestSharedPtrPerformance = 46,
    // Independent contention/capacity retry controls and growth notification
    LockFreeQueueTestRetryContract = 47,
    // Consumer scheduling interval and batch accounting
    LockFreeQueueTestConsumerYield = 48,
    // Entry spans across publication segments and reuse boundaries
    LockFreeQueueTestEntrySpan = 49
};

// Test-only control, never part of the production ABI.
class LockFreeQueueTestControl
{
public:
    // Selected whitebox pause boundary, zero disables pauses
    static std::atomic<int32_t> m_pausePoint;
    // Selected allocation/publication fault boundary
    static std::atomic<int32_t> m_failPoint;
    // Paused worker has reached the selected boundary
    static std::atomic<bool> m_reached;
    // Release the first paused worker
    static std::atomic<bool> m_release;
    // A whitebox gate exceeded its finite deadline
    static std::atomic<bool> m_timedOut;
    // Independent second publisher gate, only used by the reserved-hole test.
    static std::atomic<bool> m_secondPause;
    // Second publisher has entered its independent pause
    static std::atomic<bool> m_secondReached;
    // Release the second paused publisher
    static std::atomic<bool> m_secondRelease;
    // Injected monotonic milliseconds, UINT64_MAX selects the real clock
    static std::atomic<uint64_t> m_clock;
    // Instrumented retired-node visits
    static std::atomic<size_t> m_reclaimVisits;
    // Instrumented pool-maintenance visits
    static std::atomic<size_t> m_maintenanceVisits;
    // 仅隔离白盒副本消费的未提交失败次数
    static std::atomic<int32_t> m_transientFailures;
    // Observed calls to the cooperative yield branch
    static std::atomic<size_t> m_scheduleYields;
    // Observed calls to the sleep backoff branch
    static std::atomic<size_t> m_scheduleSleeps;

    /** Reset an isolated test's hooks
    */
    static void reset();

    /** Wait for a hook within its test deadline
    @return Whether the selected hook was reached before its deadline
    */
    static bool waitReached();

    /** Return monotonic elapsed nanoseconds
    @return Monotonic timestamp in nanoseconds
    */
    static uint64_t ticks();

    /** Read this process's handle count; zero on non-Windows hosts
    @return Process handle count, or zero when unsupported
    */
    static uint32_t handles();

    /** Read process private bytes; zero when the host has no probe
    @return Process private bytes, or zero when unsupported
    */
    static uint64_t privateBytes();

    /** Observe the same handle count for 1.5 seconds, deadline 5 seconds
    @return Stable handle count, or UINT32_MAX when the five-second deadline expires
    */
    static uint32_t settledHandles();
};

// Restores fault injection after all participating test threads have joined.
class LockFreeQueueTestFaultScope
{
public:
    /** Save the isolated test's fault controls without modifying them
    */
    LockFreeQueueTestFaultScope();

    /** Restore the saved controls, including failure and exception exits
    */
    ~LockFreeQueueTestFaultScope();

private:
    // Fault selector that preceded this scope
    int32_t m_previousPoint;
    // Remaining injected failures that preceded this scope
    int32_t m_previousFailures;

private:
    /** Do not duplicate restoration responsibility
    @param [in] other Noncopyable source scope
    */
    LockFreeQueueTestFaultScope(const LockFreeQueueTestFaultScope& other) = delete;

    /** Do not transfer restoration responsibility by assignment
    @param [in] other Noncopyable source scope
    @return Unavailable because assignment is deleted
    */
    LockFreeQueueTestFaultScope& operator=(const LockFreeQueueTestFaultScope& other) = delete;
};

// Payload with strict address alignment; test owns all live instances
#ifdef _MSC_VER
class __declspec(align(64)) LockFreeQueueTestAligned
#else
class alignas(64) LockFreeQueueTestAligned
#endif
{
public:
    // Independent payload identity checked after delivery
    int32_t m_value;
    // Quiescent-test accumulation of real payload-address alignment checks
    static bool m_valid;

    /** No default construction; check every constructed payload address
    @param [in] value Independent test payload value
    */
    explicit LockFreeQueueTestAligned(int32_t value);

    /** Validate the pool's real placement address
    @param [in] other Source object borrowed for this operation
    */
    LockFreeQueueTestAligned(const LockFreeQueueTestAligned& other);

    /** Copy out a value
    @param [in] other Source object borrowed for this operation
    @return Reference to this object after successful assignment
    */
    LockFreeQueueTestAligned& operator=(const LockFreeQueueTestAligned& other);
};

// A deliberately non-default-constructible throwing payload.
class LockFreeQueueTestValue
{
public:
    // Independent payload identity checked after delivery
    int32_t m_value;
    // Number of successfully constructed payloads not yet destroyed
    static std::atomic<int32_t> m_live;
    // Quiescent-test switch for copy-construction failure
    static bool m_throwCopy;
    // Quiescent-test switch for partial output assignment failure
    static bool m_throwAssign;

    /** Construct a counted payload
    @param [in] value Independent test payload value
    */
    explicit LockFreeQueueTestValue(int32_t value);

    /** Copy, optionally injecting failure before object lifetime begins
    @param [in] other Source object borrowed for this operation
    */
    LockFreeQueueTestValue(const LockFreeQueueTestValue& other);

    /** Modify output then optionally throw; never modify the source
    @param [in] other Source object borrowed for this operation
    @return Reference to this object after successful assignment
    */
    LockFreeQueueTestValue& operator=(const LockFreeQueueTestValue& other);

    /** Release one counted lifetime
    */
    ~LockFreeQueueTestValue();
};

#if defined(_MSC_VER) && _MSC_VER < 1900
#define LOCK_FREE_QUEUE_TEST_NOTHROW throw()
#else
#define LOCK_FREE_QUEUE_TEST_NOTHROW noexcept
#endif

// Nonthrowing move payload used to detect an unnecessary copy on dequeue
class LockFreeQueueTestMove
{
public:
    // Independent payload identity checked after delivery
    int32_t m_value;

    /** Construct the value used by the independent move oracle
    @param [in] value Independent test payload value
    */
    explicit LockFreeQueueTestMove(int32_t value);

    /** Push must still copy
    @param [in] other Source object borrowed for this operation
    */
    LockFreeQueueTestMove(const LockFreeQueueTestMove& other);

    /** This throws, detecting an unnecessary copy in the nonthrowing-move path
    @param [in] other Source object borrowed for this operation
    @return Does not return normally; always throws the test exception
    */
    LockFreeQueueTestMove& operator=(const LockFreeQueueTestMove& other);

    /** Transfer and poison the source without throwing
    @param [in,out] other Source whose value is changed by the move assignment
    @return Reference to this object after successful assignment
    */
    LockFreeQueueTestMove& operator=(LockFreeQueueTestMove&& other) LOCK_FREE_QUEUE_TEST_NOTHROW;
};

// Throwing move payload used to verify the safe copy fallback
class LockFreeQueueTestThrowMove
{
public:
    // Independent payload identity checked after delivery
    int32_t m_value;

    /** Construct a value whose move must never be chosen
    @param [in] value Independent test payload value
    */
    explicit LockFreeQueueTestThrowMove(int32_t value);

    /** Copy on push
    @param [in] other Source object borrowed for this operation
    */
    LockFreeQueueTestThrowMove(const LockFreeQueueTestThrowMove& other);

    /** Safe fallback preserving the queued source
    @param [in] other Source object borrowed for this operation
    @return Reference to this object after successful assignment
    */
    LockFreeQueueTestThrowMove& operator=(const LockFreeQueueTestThrowMove& other);

    /** Poison the source and throw if the implementation chooses incorrectly
    @param [in,out] other Source whose value is changed by the move assignment
    @return Does not return normally; poisons the source and throws
    */
    LockFreeQueueTestThrowMove& operator=(LockFreeQueueTestThrowMove&& other);
};

// All test threads are joined even if checks/allocations throw.
class LockFreeQueueTestThreads
{
public:
    // Owned worker threads joined before their borrowed state is destroyed
    std::vector<std::thread> m_threads;

    /** Join any remaining threads
    */
    ~LockFreeQueueTestThreads();

    /** Join each started thread once
    */
    void join();
};

// Callable whose copy can fail while configuring std::function.
class LockFreeQueueTestPolicy
{
public:
    // Quiescent-test switch for copy-construction failure
    static bool m_throwCopy;

    /** Construct a test policy
    */
    LockFreeQueueTestPolicy();

    /** Copy with a controlled exception
    @param [in] other Source object borrowed for this operation
    */
    LockFreeQueueTestPolicy(const LockFreeQueueTestPolicy& other);

    /** Request two extra nodes
    @param [in] context Growth snapshot borrowed only for this callback
    @return Two additional nodes
    */
    size_t operator()(const LockFreeQueueGrowContext& context) const;
};

// Stable IDs and independent test implementations, shared by all CLI selections.
class LockFreeQueueTests
{
public:
    /** Run selected cases and emit machine-readable PASS/FAIL lines
    @param [in] argc Argument count including the executable name
    @param [in] argv Non-null argument array borrowed until this call returns
    @return Zero on success; nonzero when arguments or workload validation fail
    */
    static int run(int argc, char** argv);

    /** Single producer, discard and complete FIFO oracle
    @return Whether every stated invariant passed
    */
    static bool fifo();

    /** Check actual generic atomic width, values and failed-CAS expected updates
    @return Whether every stated invariant passed
    */
    static bool atomicValues();

    /** A paused initial-batch owner cannot stop other producers or exceed capacity
    @return Whether every stated invariant passed
    */
    static bool initialBatches();

    /** Initial and grown block publication boundaries and complete physical-node reuse
    @return Whether every stated invariant passed
    */
    static bool growthTransitions();

    /** Large maintenance budgets stop after useful scans; zero/one preserve bounds
    @return Whether every stated invariant passed
    */
    static bool maintenanceBounds();

    /** 普通push及生产者上下文必须跨过任意次数未提交竞争，try入口仍一次尝试
    @return 接纳次数、顺序和剩余注入次数均符合预期
    */
    static bool retryContention();

    /** 登记槽被长期上下文占用时，关闭容量等待的 try 入队明确失败
    @return Whether every stated invariant passed
    */
    static bool registrationProgress();

    /** 私有缓存占用剩余额度时，绑定生产者关闭容量等待后明确失败
    @return Whether every stated invariant passed
    */
    static bool cacheResourceProgress();

    /** 核对竞争退避、成功路径/空轮询调度让步及普通入队的资源错误码
    @return Whether every stated invariant passed
    */
    static bool cooperativeProgress();

    /** Busy 仅表示 try 竞争；真实满额和状态错误必须独立，失败后可恢复
    @return Whether every stated invariant passed
    */
    static bool busyContract();

    /** Multiple producers: independent sequence/ID/accepted-set oracle
    @param [in] producers Number of producer threads; caller supplies a positive count
    @param [in] count Items per producer; total workload is producers multiplied by count
    @param [in] bounded Use a finite small pool to force capacity pressure
    @return Whether every stated invariant passed
    */
    static bool mpsc(size_t producers, size_t count, bool bounded);

    /** Verify the bounded cache and concurrent group sharing
    @return Whether every stated invariant passed
    */
    static bool cacheReuse();

    /** Pause a cache ownership transfer and exercise one-visit maintenance
    @return Whether every stated invariant passed
    */
    static bool cacheInterleavings();

    /** Check modular counts, leased-record handoff and concurrent observers
    @return Whether every stated invariant passed
    */
    static bool counters();

    /** Repeat lifecycle transitions and independent instances
    @return Whether every stated invariant passed
    */
    static bool lifecycle();

    /** Immediate shared lifetime release and caller-owned raw pointers
    @return Whether every stated invariant passed
    */
    static bool ownership();

    /** Legal clear/push concurrency and payload cleanup
    @return Whether every stated invariant passed
    */
    static bool concurrentClear();

    /** Repeated reuse without new system allocations
    @return Whether every stated invariant passed
    */
    static bool reuse();

    /** Non-default construction and copy/assignment failures
    @return Whether every stated invariant passed
    */
    static bool exceptions();

    /** Verify 64-byte aligned, non-default-constructible inline storage
    @return Whether every stated invariant passed
    */
    static bool alignment();

    /** Invalid config, max=1 and uninitialized API results
    @return Whether every stated invariant passed
    */
    static bool boundaries();

    /** Threshold 0/positive, cap and speculative failure
    @return Whether every stated invariant passed
    */
    static bool thresholds();

    /** Policy return values/context/copy failure/saturation
    @return Whether every stated invariant passed
    */
    static bool policies();

    /** Each real pool allocation point, rollback and retry
    @return Whether every stated invariant passed
    */
    static bool allocations();

    /** Pause one producer at a publication boundary
    @param [in] point Whitebox publication boundary selected for the pause
    @return Whether every stated invariant passed
    */
    static bool publication(int32_t point);

    /** Resume the paused producer before consuming; preserve actual publication order
    @return Whether every stated invariant passed
    */
    static bool publicationOrder();

    /** Paused segment publishers, repeated reuse and structural exhaustion
    @return Whether every stated invariant passed
    */
    static bool segmentReuse();

    /** Bounded cached batches, paused ownership and complete reuse at the limit
    @return Whether every stated invariant passed
    */
    static bool cachedBatches();

    /** Payload contracts and canonical policy state across capacity modes
    @return Whether every stated invariant passed
    */
    static bool capacityContracts();

    /** Bound producer lease reuse, explicit release and synchronized handoff
    @return Whether every stated invariant passed
    */
    static bool reservedProducer();

    /** Concurrent reservation collisions and post-publication pause
    @return Whether every stated invariant passed
    */
    static bool reservations();

    /** Dynamic idle time, multiple blocks, minimum, regrowth
    @return Whether every stated invariant passed
    */
    static bool shrink();

    /** Pool traversal/closing/reclamation interleavings
    @return Whether every stated invariant passed
    */
    static bool poolInterleavings();

    /** Pause the sole consumer in copy/reclamation/block closure
    @return Whether every stated invariant passed
    */
    static bool pausedConsumer();

    /** One bounded queue throughout a timed window; exact per-lane accepted sets
    @param [in] seconds Active workload duration in seconds
    @param [in] producerSession Bind one producer context per worker
    @param [in] dynamicPool Use threshold one on the finite 256-node pool; no growth is permitted
    @param [in] prefixPool Use an 8192-node initial block with unlimited configured capacity but a policy declining growth; consumption remains single-item pop
    @return Whether every stated invariant passed
    */
    static bool steady(uint32_t seconds, bool producerSession = false, bool dynamicPool = false, bool prefixPool = false);

    /** Set a small isolated pool; lifecycle is quiescent
    @param [in,out] queue Externally quiescent queue whose pool configuration is replaced
    @param [in] initial Initial physical node count, including the sentinel
    @param [in] maximum Physical node limit, or -1 for no configured limit
    @param [in] threshold Free-node level that triggers proactive growth
    @param [in] producers Number of producer threads; caller supplies a positive count
    @return Whether every stated invariant passed
    */
    static bool configure(LockFreeQueue<int32_t>& queue, size_t initial, int64_t maximum,
        size_t threshold = 0, size_t producers = 16);

    /** Map registry ID to a real test implementation
    @param [in] id Registered core case identifier
    @return Whether every stated invariant passed
    */
    static bool dispatch(int32_t id);
};