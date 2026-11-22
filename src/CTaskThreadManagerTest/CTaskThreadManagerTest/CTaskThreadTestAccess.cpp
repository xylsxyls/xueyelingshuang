#include "CTaskThreadTestAccess.h"
#ifndef CTTM_TEST_DLL
#include <memory>
#include "CTaskPrivateTestAccess.h"
#include "BlackBoxTestSupport.h"
#include <climits>
#include <stdexcept>

CTaskTestThreadState::Type CTaskTestThreadState::m_member = nullptr;
CTaskTestManagerState::Type CTaskTestManagerState::m_member = nullptr;
CTaskTestManagerId::Type CTaskTestManagerId::m_member = nullptr;
CTaskTestManagerThreads::Type CTaskTestManagerThreads::m_member = nullptr;
CTaskTestSequence::Type CTaskTestSequence::m_member = nullptr;
CTaskTestCurrent::Type CTaskTestCurrent::m_member = nullptr;
CTaskTestReleased::Type CTaskTestReleased::m_member = nullptr;
CTaskTestQueued::Type CTaskTestQueued::m_member = nullptr;
CTaskTestDequeued::Type CTaskTestDequeued::m_member = nullptr;
CTaskTestCapacityPressure::Type CTaskTestCapacityPressure::m_member = nullptr;
CTaskTestObservers::Type CTaskTestObservers::m_member = nullptr;
CTaskTestBuckets::Type CTaskTestBuckets::m_member = nullptr;
CTaskTestDomain::Type CTaskTestDomain::m_member = nullptr;
CTaskTestStorage::Type CTaskTestStorage::m_member = nullptr;
CTaskTestPreempt::Type CTaskTestPreempt::m_member = nullptr;
CTaskTestWakeup::Type CTaskTestWakeup::m_member = nullptr;
CTaskTestCompletion::Type CTaskTestCompletion::m_member = nullptr;
CTaskTestPoolChanged::Type CTaskTestPoolChanged::m_member = nullptr;
CTaskTestSemaphoreWake::Type CTaskTestSemaphoreWake::m_member = nullptr;

// C++ 显式模板实例化允许指定私有成员；只保存指针，不依赖对象布局偏移。
template struct CTaskPrivateBinding<CTaskTestThreadState, &CTaskThread::m_state>;
template struct CTaskPrivateBinding<CTaskTestManagerState, &CTaskThreadManager::m_state>;
template struct CTaskPrivateBinding<CTaskTestManagerId, &CTaskManagerState::m_threadId>;
template struct CTaskPrivateBinding<CTaskTestManagerThreads, &CTaskManagerState::m_threads>;
template struct CTaskPrivateBinding<CTaskTestSequence, &CTaskThreadState::m_sequence>;
template struct CTaskPrivateBinding<CTaskTestCurrent, &CTaskThreadState::m_current>;
template struct CTaskPrivateBinding<CTaskTestReleased, &CTaskThreadState::m_releasedTasks>;
template struct CTaskPrivateBinding<CTaskTestQueued, &CTaskThreadState::m_queued>;
template struct CTaskPrivateBinding<CTaskTestDequeued, &CTaskThreadState::m_dequeuedTasks>;
template struct CTaskPrivateBinding<CTaskTestCapacityPressure, &CTaskThreadState::m_pressureYields>;
template struct CTaskPrivateBinding<CTaskTestObservers, &CTaskThreadState::m_observers>;
template struct CTaskPrivateBinding<CTaskTestBuckets, &CTaskThreadState::m_buckets>;
template struct CTaskPrivateBinding<CTaskTestDomain, &CTaskThreadState::m_readDomain>;
template struct CTaskPrivateBinding<CTaskTestStorage, &CTaskThreadState::m_storage>;
template struct CTaskPrivateBinding<CTaskTestPreempt, &CTaskThreadState::preempt>;
template struct CTaskPrivateBinding<CTaskTestWakeup, &CTaskThreadState::m_wakeup>;
template struct CTaskPrivateBinding<CTaskTestCompletion, &CTaskThreadState::m_completion>;
template struct CTaskPrivateBinding<CTaskTestPoolChanged, &CTaskPoolState::m_changed>;
template struct CTaskPrivateBinding<CTaskTestSemaphoreWake, &Semaphore::m_wake>;

typedef LockFreeQueue<CTaskStorageSlot*> CTaskBoundQueue;
typedef LockFreeQueueChannel<LockFreeQueueArenaNode<CTaskStorageSlot*>, LockFreeQueueChannelPool<CTaskStorageSlot*>,
    LockFreeQueueArenaRecord<CTaskStorageSlot*>> CTaskBoundChannel;
typedef LockFreeQueueArena<CTaskStorageSlot*> CTaskBoundArena;
template struct CTaskPrivateBinding<CTaskTestQueueSetting<CTaskBoundArena, 7>, &CTaskBoundQueue::m_arena>;
template struct CTaskPrivateBinding<CTaskTestQueueSetting<LockFreeQueuePoolConfig, 8, CTaskBoundArena>, &CTaskBoundArena::m_config>;
template struct CTaskPrivateBinding<CTaskTestQueueSetting<CTaskBoundChannel, 5>, &CTaskBoundQueue::m_channel>;
template struct CTaskPrivateBinding<CTaskTestQueueSetting<uint32_t, 6, CTaskBoundChannel>, &CTaskBoundChannel::m_consumerInterval>;
template struct CTaskPrivateBinding<CTaskTestQueueSetting<bool, 0>, &CTaskBoundQueue::m_distributedCounters>;
template struct CTaskPrivateBinding<CTaskTestQueueSetting<bool, 9>, &CTaskBoundQueue::m_cancelableReservations>;
template struct CTaskPrivateBinding<CTaskTestQueueSetting<uint32_t, 1>, &CTaskBoundQueue::m_capacityRetriesBeforeSleep>;
template struct CTaskPrivateBinding<CTaskTestQueueSetting<uint32_t, 2>, &CTaskBoundQueue::m_capacitySleepMilliseconds>;
template struct CTaskPrivateBinding<CTaskTestQueueSetting<uint32_t, 3>, &CTaskBoundQueue::m_maintenanceInterval>;
template struct CTaskPrivateBinding<CTaskTestQueueSetting<size_t, 4>, &CTaskBoundQueue::m_maintenanceBudget>;

void CTaskThreadTestAccess::verifyQueueSettings(const CTaskThreadConfig& config, int32_t level)
{
    CTaskPriorityBucket bucket(level);
    bbCheck(bucket.initialize(config).m_committed, "configured bucket initialization failed");
    const CTaskQueueConfig& settings = config.priorityQueueConfig(level);
    CTaskBoundQueue& queue = bucket.m_queue;
    const CTaskBoundArena& arena = queue.*CTaskTestQueueSetting<CTaskBoundArena, 7>::m_member;
    bbCheck(arena.entryOnly() == settings.m_entryOnly && bucket.m_entryOnly == settings.m_entryOnly &&
        queue.*CTaskTestQueueSetting<bool, 9>::m_member == settings.m_cancelableReservations,
        "queue payload or reservation mode differs from selected configuration");
    const LockFreeQueuePoolConfig& nativePool = arena.*CTaskTestQueueSetting<LockFreeQueuePoolConfig, 8, CTaskBoundArena>::m_member;
    bbCheck(nativePool.initialCapacity == settings.m_queue.initialCapacity && nativePool.maximumCapacity == settings.m_queue.maximumCapacity &&
        nativePool.growThreshold == settings.m_queue.growThreshold && nativePool.producerYieldInterval == settings.m_queue.producerYieldInterval &&
        nativePool.maximumProducers == settings.m_queue.maximumProducers, "native FIFO pool differs from selected settings");
    // 按所选级别的上限实际登记，额外申请失败后，归还一项应允许再次登记。
    const size_t limit = settings.m_queue.maximumProducers;
    std::unique_ptr<LockFreeQueueProducer<CTaskStorageSlot*>[]> producers(new LockFreeQueueProducer<CTaskStorageSlot*>[limit]);
    for (size_t i = 0; i < limit; ++i)
    {
        bbCheck(queue.tryAcquireProducer(producers[i]) == LockFreeQueueSuccess, "producer registration rejected before configured limit");
    }
    LockFreeQueueProducer<CTaskStorageSlot*> extra;
    bbCheck(queue.tryAcquireProducer(extra) == LockFreeQueueCapacityLimit, "native FIFO ignored producer registration limit");
    producers[0].release();
    bbCheck(queue.tryAcquireProducer(extra) == LockFreeQueueSuccess, "returned producer registration was not reusable");
    const CTaskBoundChannel& channel = queue.*CTaskTestQueueSetting<CTaskBoundChannel, 5>::m_member;
    bbCheck(channel.*CTaskTestQueueSetting<uint32_t, 6, CTaskBoundChannel>::m_member == settings.m_consumerYieldInterval &&
        queue.*CTaskTestQueueSetting<bool, 0>::m_member == settings.m_distributedCounters &&
        queue.*CTaskTestQueueSetting<uint32_t, 1>::m_member == settings.m_capacityRetries &&
        queue.*CTaskTestQueueSetting<uint32_t, 2>::m_member == settings.m_capacitySleepMilliseconds &&
        queue.*CTaskTestQueueSetting<uint32_t, 3>::m_member == settings.m_maintenanceInterval &&
        queue.*CTaskTestQueueSetting<size_t, 4>::m_member == settings.m_maintenanceBudget, "native FIFO settings differ from selected configuration");
}

void CTaskThreadTestAccess::semaphoreInitializationTest()
{
    const SemaphoreStatus failures[] = {SEMAPHORE_NO_MEMORY, SEMAPHORE_UNSUPPORTED, SEMAPHORE_SYSTEM_ERROR};
    const CTaskThreadStatus expected[] = {CTaskThreadNoMemory, CTaskThreadUnsupported, CTaskThreadSystemFailure};
    CTaskThreadConfig threadConfig;
    CTaskThreadPoolConfig poolConfig;
    poolConfig.m_initialThreads = 1;
    poolConfig.m_maximumThreads = 1;
    SemaphoreConfig* settings[] = {&threadConfig.m_wakeupSemaphore, &threadConfig.m_completionSemaphore, &poolConfig.m_changeSemaphore};
    for (size_t i = 0; i < 3; ++i)
    {
        SemaphoreWaitPolicy policy;
        const uint32_t offset = static_cast<uint32_t>(i);
        policy.m_promotionMilliseconds = offset + 5;
        policy.m_localSlice = offset + 7;
        policy.m_mobileSlice = offset + 2;
        policy.m_posixSlice = offset + 4;
        policy.m_capacityAttempts = offset + 9;
        policy.m_capacitySleep = offset + 3;
        policy.m_contentionYieldInterval = offset + 11;
        policy.m_initialPauses = offset + 13;
        policy.m_maximumPauses = offset + 17;
        bbCheck(settings[i]->setWaitPolicy(policy).status == SEMAPHORE_OK, "notification wait policy rejected");
        policy.m_localSlice = 0;
        bbCheck(settings[i]->setWaitPolicy(policy).status == SEMAPHORE_INVALID, "invalid replacement wait policy accepted");
        settings[i]->configureEventPool().initialCapacity = i + 2;
        settings[i]->configureCountPool().initialCapacity = i + 5;
    }
    for (size_t failure = 0; failure < 3; ++failure)
    {
        const int32_t nativeError = failures[failure] == SEMAPHORE_SYSTEM_ERROR ? 1234 : 0;
        for (size_t kind = 0; kind < 3; ++kind)
        {
            std::unique_ptr<CTaskThreadState> threadOwner(new CTaskThreadState(threadConfig));
            CTaskThreadState& thread = *threadOwner;
            CTaskPoolState pool(poolConfig);
            Semaphore& notification = kind == 0 ? thread.*CTaskTestWakeup::m_member :
                (kind == 1 ? thread.*CTaskTestCompletion::m_member : pool.*CTaskTestPoolChanged::m_member);
            const SemaphoreWaitPolicy& policy = notification.waitPolicy();
            bbCheck(policy.m_promotionMilliseconds == kind + 5 && policy.m_localSlice == kind + 7 &&
                policy.m_mobileSlice == kind + 2 && policy.m_posixSlice == kind + 4 &&
                policy.m_capacityAttempts == kind + 9 && policy.m_capacitySleep == kind + 3 &&
                policy.m_contentionYieldInterval == kind + 11 && policy.m_initialPauses == kind + 13 && policy.m_maximumPauses == kind + 17 &&
                notification.eventPoolStatistics().capacity == kind + 2 && notification.countPoolStatistics().capacity == kind + 5,
                "independent notification settings did not reach the native instance");
            (notification.*CTaskTestSemaphoreWake::m_member).failInitialization(failures[failure], nativeError);
            bbCheck(notification.initializationResult().status == failures[failure], "semaphore fault fixture not applied");
            const CTaskThreadResult result = kind == 2 ? pool.start() : thread.start();
            bbCheck(result.m_status == expected[failure] && !result.m_committed && result.m_nativeError == nativeError,
                "semaphore initialization failure was accepted or lost its diagnosis");
            bbCheck(CTaskThreadManager::instance().count() == 0, "failed semaphore startup registered a worker");
        }
    }
    // 注入只作用于被测实例；新实例仍可启动并正常结束，不能用全局失败掩盖回滚问题。
    std::unique_ptr<CTaskThreadState> threadOwner(new CTaskThreadState(threadConfig));
    CTaskThreadState& thread = *threadOwner;
    bbCheck(thread.start().m_status == CTaskThreadOk, "fresh worker failed after semaphore fault");
    bbCheck(thread.finish(true).m_status == CTaskThreadOk && thread.statistics().m_exited,
        "fresh worker did not join after semaphore fault");
    CTaskPoolState pool(poolConfig);
    bbCheck(pool.start().m_status == CTaskThreadOk, "fresh pool failed after semaphore fault");
    bbCheck(pool.shutdown(true).m_status == CTaskThreadOk && CTaskThreadManager::instance().count() == 0,
        "fresh pool did not unregister after semaphore fault");
}

/** 从生产门面取得状态，只在测试工程使用
@param [in,out] thread 已成功初始化的线程
@return 线程生命周期内有效的私有状态
*/
static CTaskThreadState& TestState(CTaskThread& thread)
{
    return *(thread.*CTaskTestThreadState::m_member);
}

/** 通过公开配置入口按需初始化管理器，再读取白盒状态
@return 管理器状态；公开初始化失败时为 nullptr
*/
static CTaskManagerState* TestManagerState()
{
    if ((CTaskThreadManager::instance().*CTaskTestManagerState::m_member).load(std::memory_order_acquire) == nullptr)
    {
        CTaskThreadManager::instance().configure(CTaskThreadManagerConfig());
    }
    return (CTaskThreadManager::instance().*CTaskTestManagerState::m_member).load(std::memory_order_acquire);
}

bool CTaskThreadTestAccess::holdManagerRead(const std::function<void()>& held)
{
    CTaskManagerState* state = TestManagerState();
    bool called = false;
    if (state != nullptr)
    {
        (state->*CTaskTestManagerThreads::m_member).forEach([&held, &called](const uint32_t&, const std::shared_ptr<CTaskThread>&)
        {
            if (!called)
            {
                held();
                // forEach 会捕获回调异常，只有正常返回才算夹具完成
                called = true;
            }
        });
    }
    return called;
}

void CTaskThreadTestAccess::setTaskSequence(CTaskThread& thread, uint64_t sequence)
{
    const uint64_t block = sequence / 512;
    (TestState(thread).*CTaskTestSequence::m_member).store(block + (sequence % 512 != 0 ? 1 : 0), std::memory_order_relaxed);
    CTaskReadDomain& domain = TestState(thread).*CTaskTestDomain::m_member;
    CTaskStorage& storage = TestState(thread).*CTaskTestStorage::m_member;
    for (size_t i = 0; i <= domain.capacity(); ++i)
    {
        storage.cache(i).m_nextSequence = 0;
        storage.cache(i).m_lastSequence = 0;
    }
    if (sequence % 512 != 0 && sequence != UINT64_MAX)
    {
        // 边界用例随后由同一测试线程投递；部分尾段只交给一个位置，不能复制给所有缓存。
        CTaskReadPosition* position = domain.enter();
        if (position == nullptr)
        {
            throw std::runtime_error("sequence fixture cannot acquire idle read position");
        }
        CTaskStorageCache& cache = storage.cache(domain.indexOf(*position));
        cache.m_nextSequence = sequence;
        cache.m_lastSequence = block == UINT64_MAX / 512 ? UINT64_MAX : (block + 1) * 512;
        domain.leave(*position);
    }
}

void CTaskThreadTestAccess::seedPendingCounters(CTaskThread& thread, bool unlimited)
{
    CTaskThreadState& state = TestState(thread);
    const uint64_t offset = UINT64_MAX - 2;
    if (unlimited)
    {
        // 门控任务已入队并被取走。给真实的生产累计量和消费累计量加同一偏移，保留净值。
        CTaskStorageCache& cache = (state.*CTaskTestStorage::m_member).cache(0);
        cache.m_reserved.fetch_add(offset, std::memory_order_relaxed);
        cache.m_published.fetch_add(offset, std::memory_order_relaxed);
        (state.*CTaskTestReleased::m_member).fetch_add(offset, std::memory_order_release);
    }
    else
    {
        (state.*CTaskTestQueued::m_member).fetch_add(offset, std::memory_order_relaxed);
    }
    (state.*CTaskTestDequeued::m_member).fetch_add(offset, std::memory_order_release);
}

void CTaskThreadTestAccess::setManagerSequence(uint32_t sequence)
{
    CTaskManagerState* state = TestManagerState();
    if (state != nullptr)
    {
        (state->*CTaskTestManagerId::m_member).store(sequence, std::memory_order_relaxed);
    }
}

size_t CTaskThreadTestAccess::recordCount(CTaskThread& thread)
{
    CTaskAdmission& observers = TestState(thread).*CTaskTestObservers::m_member;
    CTaskReadDomain& domain = TestState(thread).*CTaskTestDomain::m_member;
    const bool observing = observers.enter();
    CTaskReadPosition* reader = nullptr;
    size_t count = 0;
    if (!observing && !thread.statistics().m_exited)
    {
        throw std::runtime_error("record inspection raced with worker cleanup");
    }
    if (observing)
    {
        reader = domain.enter();
        if (reader == nullptr)
        {
            observers.leave();
            throw std::runtime_error("record inspection has no read protection");
        }
    }
    try
    {
        // exited 在存储清理后发布；关闭入口不能替代对真实残留记录的检查。
        (TestState(thread).*CTaskTestStorage::m_member).forEach([&count](CTaskStorageSlot&)
        {
            ++count;
        });
    }
    catch (...)
    {
        if (observing)
        {
            domain.leave(*reader);
            observers.leave();
        }
        throw;
    }
    if (observing)
    {
        domain.leave(*reader);
        observers.leave();
    }
    return count;
}

size_t CTaskThreadTestAccess::bucketCount(CTaskThread& thread)
{
    if (!(TestState(thread).*CTaskTestObservers::m_member).enter())
    {
        if (!thread.statistics().m_exited)
        {
            throw std::runtime_error("bucket inspection raced with worker cleanup");
        }
        return (TestState(thread).*CTaskTestBuckets::m_member).size();
    }
    const size_t count = (TestState(thread).*CTaskTestBuckets::m_member).size();
    (TestState(thread).*CTaskTestObservers::m_member).leave();
    return count;
}

bool CTaskThreadTestAccess::capacityPressure(CTaskThread& thread)
{
    return (TestState(thread).*CTaskTestCapacityPressure::m_member).load(std::memory_order_acquire) != 0;
}

void CTaskThreadTestAccess::resetCapacityPressure(CTaskThread& thread)
{
    (TestState(thread).*CTaskTestCapacityPressure::m_member).store(0, std::memory_order_release);
}

bool CTaskThreadTestAccess::closeIdleBucket(CTaskThread& thread, int32_t level, const std::shared_ptr<CTask>& interveningTask)
{
    if (!(TestState(thread).*CTaskTestObservers::m_member).enter())
    {
        return false;
    }
    std::shared_ptr<CTaskPriorityBucket> bucket;
    LockFreeMapToken token;
    const bool empty = (TestState(thread).*CTaskTestBuckets::m_member).lockFreeMapGet(level, bucket, token) == LOCK_FREE_MAP_OK &&
        bucket->m_queued.load() == 0;
    // 保留真实桶的强引用后即可归还观察登记；下一次投递必须通过正常公开入口。
    (TestState(thread).*CTaskTestObservers::m_member).leave();
    bbCheck(empty, "fixture did not observe an empty bucket");
    if (interveningTask)
    {
        const CTaskThreadResult result = thread.postTaskResult(interveningTask, level);
        bbCheck(result.m_status == CTaskThreadOk && result.m_committed, "interleaved post did not commit");
    }
    const bool closed = bucket->m_writers.closeIfEmpty();
    if (closed && interveningTask)
    {
        // 旧实现若错关有货的桶，先恢复现场，让夹具仍能排空并报告真正失败的断言。
        bucket->m_writers.reopen();
    }
    return closed;
}

CTaskThreadResult CTaskThreadTestAccess::latePreempt(CTaskThread& thread)
{
    CTaskReadDomain& domain = TestState(thread).*CTaskTestDomain::m_member;
    CTaskReadPosition* reader = domain.enter();
    if (reader == nullptr)
    {
        return CTaskThreadResult(CTaskThreadRetry);
    }
    CTaskStorageSlot posted;
    CTaskStorageState postedState;
    posted.m_state = &postedState;
    posted.m_state->m_bits.store(3);
    const CTaskThreadStatus status = (TestState(thread).*CTaskTestPreempt::m_member)(INT_MAX, &posted);
    domain.leave(*reader);
    return CTaskThreadResult(status);
}

CTaskThreadResult CTaskThreadTestAccess::holdDirectoryRead(CTaskThread& thread, const std::function<void()>& held)
{
    CTaskAdmission& observers = TestState(thread).*CTaskTestObservers::m_member;
    if (!observers.enter())
    {
        return CTaskThreadResult(CTaskThreadClosed);
    }
    CTaskReadDomain& domain = TestState(thread).*CTaskTestDomain::m_member;
    CTaskReadPosition* reader = domain.enter();
    if (reader == nullptr)
    {
        observers.leave();
        return CTaskThreadResult(CTaskThreadRetry);
    }
    CTaskThreadResult result;
    try
    {
        held();
    }
    catch (...)
    {
        result = CTaskThreadResult(CTaskThreadCallbackFailure);
    }
    domain.leave(*reader);
    observers.leave();
    return result;
}

#endif