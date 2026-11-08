#include "CTaskThreadState.h"
#include "CTaskCallbackScope.h"
#include "CTaskThreadPriority.h"
#include "CTaskResultAdapter.h"
#include <algorithm>
#include <climits>
#include <set>
#include <limits>
#include <system_error>

/** 查询写预取指令是否可用；不支持时仍使用原有读预取
@return 本编译目标和当前处理器是否支持写预取提示
*/
static bool TaskWritePrefetchAvailable()
{
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    int registers[4] = {};
    __cpuid(registers, static_cast<int>(0x80000000u));
    if (static_cast<uint32_t>(registers[0]) < 0x80000001u)
    {
        return false;
    }
    __cpuid(registers, static_cast<int>(0x80000001u));
    return (registers[2] & (1 << 8)) != 0;
#elif defined(__GNUC__) || defined(__clang__)
    return true;
#else
    return false;
#endif
}

/** 预取仍由任务池持有的稳定地址，不执行任务、不改变队列状态
@param [in] address 在当前消费者访问期间有效的地址
@param [in] writeAccess 是否预备写入，Windows 调用方必须先检查写预取能力
*/
static inline void PrefetchTaskAddress(const void* address, bool writeAccess = false)
{
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    if (writeAccess)
    {
        _m_prefetchw(address);
    }
    else
    {
        _mm_prefetch(static_cast<const char*>(address), _MM_HINT_T0);
    }
#elif defined(__GNUC__) || defined(__clang__)
    if (writeAccess)
    {
        __builtin_prefetch(address, 1, 3);
    }
    else
    {
        __builtin_prefetch(address, 0, 3);
    }
#else
    (void)address;
    (void)writeAccess;
#endif
}

CTaskThreadState::CTaskThreadState(const CTaskThreadConfig& config) :
m_config(config),
m_accepting(true),
m_buckets(false),
m_pinCommon(config.m_priorityMap.nodes.maximumCapacity == -1 && config.m_priorityMap.values.maximumCapacity == -1),
m_writePrefetch(TaskWritePrefetchAvailable()),
m_scanBuckets(false),
m_storage(m_readDomain),
m_started(0),
m_stopMode(0),
m_joining(false),
m_exited(false),
m_joined(false),
m_sequence(0),
m_current(nullptr),
m_currentLevel(0),
m_floorUses(0),
m_floorMinimum(INT_MAX),
m_preemptionFloor(1),
m_outstanding(0),
m_queued(0),
m_releasedTasks(0),
m_dequeuedTasks(0),
m_executed(0),
m_rejected(0),
m_errors(0),
m_wakePending(false),
m_trimBuckets(false)
{

}

CTaskThreadState::~CTaskThreadState()
{
    if (m_thread.joinable())
    {
        // 所有权由管理器保留到 join 成功；这里仅处理初始化回滚和静止析构。
        finish(false);
    }
}

CTaskThreadResult CTaskThreadState::start()
{
    if (m_config.m_maximumTasks < -1 || m_config.m_idleCheckMilliseconds == 0 ||
        m_config.m_idleCheckMilliseconds > 60000 || m_config.m_taskBuckets == 0 ||
        m_config.m_systemPriority < CTaskPriorityInherited || m_config.m_systemPriority > CTaskPriorityHighest ||
        m_config.m_priorityMap.operationRecordLimit < 4 || m_config.m_taskMap.protectionRecordCount < 4)
    {
        return CTaskThreadResult(CTaskThreadInvalid);
    }
    if (!m_accepting.is_lock_free() || !m_current.is_lock_free() || !m_started.is_lock_free() || !m_joined.is_lock_free() || !m_readDomain.atomicsLockFree())
    {
        return CTaskThreadResult(CTaskThreadUnsupported);
    }
    CTaskThreadResult result = CTaskResultAdapter::map(m_buckets.lockFreeMapConfigure(m_config.m_priorityMap));
    if (result.m_status != CTaskThreadOk)
    {
        return result;
    }
    if (!m_readDomain.initialize(m_config.m_taskMap.protectionRecordCount))
    {
        return CTaskThreadResult(CTaskThreadNoMemory);
    }
    result = m_storage.initialize(m_config.m_taskMap);
    if (result.m_status != CTaskThreadOk)
    {
        return result;
    }
    // 初始化时验证 FIFO 参数，避免直到第一次投递才暴露不可用的配置。
    try
    {
        std::shared_ptr<CTaskPriorityBucket> initial(new CTaskPriorityBucket(1));
        result = initial->initialize(m_config);
        if (result.m_status != CTaskThreadOk)
        {
            return result;
        }
        result = CTaskResultAdapter::map(m_buckets.lockFreeMapSet(1, initial));
        if (!result.m_committed)
        {
            return result;
        }
        m_cachedBuckets[0].publish(initial);
        m_thread = std::thread(&CTaskThreadState::run, this);
        result = CTaskThreadPriority::apply(m_thread, m_config.m_systemPriority);
        m_started.store(result.m_status == CTaskThreadOk ? 1 : 2, std::memory_order_release);
        wake();
        if (result.m_status != CTaskThreadOk)
        {
            finish(false);
            return result;
        }
        return CTaskThreadResult(CTaskThreadOk, true);
    }
    catch (const std::bad_alloc&)
    {
        result = CTaskThreadResult(CTaskThreadNoMemory);
    }
    catch (const std::system_error& error)
    {
        result = CTaskThreadResult(CTaskThreadSystemFailure, false, error.code().value());
    }
    catch (...)
    {
        result = CTaskThreadResult(CTaskThreadCallbackFailure);
    }
    m_started.store(2, std::memory_order_release);
    wake();
    if (m_thread.joinable())
    {
        finish(false);
    }
    return result;
}

uint64_t CTaskThreadState::nextSequence(CTaskStorageCache& cache)
{
    if (cache.m_nextSequence != cache.m_lastSequence)
    {
        return ++cache.m_nextSequence;
    }
    uint64_t value = m_sequence.load(std::memory_order_relaxed);
    while (value != UINT64_MAX)
    {
        const uint64_t count = (std::min)(UINT64_MAX - value, UINT64_C(64));
        if (m_sequence.compare_exchange_weak(value, value + count, std::memory_order_relaxed, std::memory_order_relaxed))
        {
            cache.m_nextSequence = value + 1;
            cache.m_lastSequence = value + count;
            return value + 1;
        }
    }
    return 0;
}

bool CTaskThreadState::reserveTask(CTaskStorageCache& cache)
{
    if (m_config.m_maximumTasks == -1)
    {
        cache.m_reserved.store(cache.m_reserved.load(std::memory_order_relaxed) + 1, std::memory_order_release);
        return true;
    }
    const uint64_t limit = m_config.m_maximumTasks == -1 ? UINT64_MAX - 1 : static_cast<uint64_t>(m_config.m_maximumTasks);
    for (;;)
    {
        // 归还发布包含此前的预留；按此顺序读取，不会把较新的归还减去较旧的预留。
        const uint64_t released = m_releasedTasks.load(std::memory_order_acquire);
        uint64_t count = m_outstanding.load(std::memory_order_relaxed);
        if (count - released >= limit)
        {
            return false;
        }
        if (m_outstanding.compare_exchange_weak(count, count + 1, std::memory_order_acq_rel, std::memory_order_relaxed))
        {
            return true;
        }
    }
}

uint64_t CTaskThreadState::outstandingCount() const
{
    for (;;)
    {
        const uint64_t released = m_releasedTasks.load(std::memory_order_acquire);
        const uint64_t reserved = m_config.m_maximumTasks == -1 ? m_storage.reservedCount() : m_outstanding.load(std::memory_order_acquire);
        if (released == m_releasedTasks.load(std::memory_order_acquire))
        {
            return reserved - released;
        }
        // 只因消费者已经推进而重读，不等待任何特定线程归还私有状态。
    }
}

uint64_t CTaskThreadState::queuedCount() const
{
    for (;;)
    {
        const uint64_t dequeued = m_dequeuedTasks.load(std::memory_order_acquire);
        const uint64_t queued = m_config.m_maximumTasks == -1 ? m_storage.publishedCount() : m_queued.load(std::memory_order_acquire);
        if (dequeued == m_dequeuedTasks.load(std::memory_order_acquire))
        {
            return queued - dequeued;
        }
    }
}

uint64_t CTaskThreadState::bucketQueuedCount(const CTaskPriorityBucket& bucket) const
{
    if (!m_pinCommon || bucket.m_level > 16)
    {
        return bucket.m_queued.load(std::memory_order_acquire);
    }
    for (;;)
    {
        const uint64_t taken = bucket.m_taken.load(std::memory_order_acquire);
        const uint64_t published = m_storage.publishedCount(bucket.m_level);
        if (taken == bucket.m_taken.load(std::memory_order_acquire))
        {
            return published - taken;
        }
    }
}

void CTaskThreadState::countPublication(CTaskStorageCache& cache, CTaskPriorityBucket& bucket, bool add)
{
    const uint64_t amount = add ? UINT64_C(1) : UINT64_MAX;
    if (m_config.m_maximumTasks == -1)
    {
        cache.m_published.store(cache.m_published.load(std::memory_order_relaxed) + amount, std::memory_order_release);
    }
    else
    {
        m_queued.fetch_add(amount, std::memory_order_acq_rel);
    }
    if (m_pinCommon && bucket.m_level <= 16)
    {
        std::atomic<uint64_t>& levelCount = cache.m_levels[bucket.m_level - 1];
        levelCount.store(levelCount.load(std::memory_order_relaxed) + amount, std::memory_order_release);
    }
    else
    {
        bucket.m_queued.fetch_add(amount, std::memory_order_relaxed);
    }
}

CTaskThreadResult CTaskThreadState::acquireBucket(int32_t level, std::shared_ptr<CTaskPriorityBucket>& bucket)
{
    if (level <= 16 && m_cachedBuckets[level - 1].get(bucket) && bucket->m_writers.enter())
    {
        return CTaskThreadResult();
    }
    std::shared_ptr<CTaskPriorityBucket> candidate;
    for (;;)
    {
        LockFreeMapToken token;
        LockFreeMapStatus status = m_buckets.lockFreeMapGet(level, bucket, token);
        if (status == LOCK_FREE_MAP_NOT_FOUND)
        {
            if (!candidate)
            {
                candidate.reset(new CTaskPriorityBucket(level));
                CTaskThreadResult result = candidate->initialize(m_config);
                if (result.m_status != CTaskThreadOk)
                {
                    return result;
                }
            }
            status = m_buckets.lockFreeMapGetOrInsert(level, candidate, bucket, token);
        }
        if (status == LOCK_FREE_MAP_CONFLICT)
        {
            continue;
        }
        if (status != LOCK_FREE_MAP_OK && status != LOCK_FREE_MAP_COMMITTED && status != LOCK_FREE_MAP_EXISTS)
        {
            if (status == LOCK_FREE_MAP_CAPACITY || status == LOCK_FREE_MAP_RESERVED)
            {
                m_trimBuckets.store(true, std::memory_order_release);
                wake();
            }
            return CTaskResultAdapter::map(status);
        }
        if (bucket->m_writers.enter())
        {
            if (level > 16 || !m_cachedBuckets[level - 1].publish(bucket))
            {
                // 必须早于本次 FIFO 发布；缓存竞争失败者直接走通用路径，不等初始化者。
                m_scanBuckets.store(true, std::memory_order_release);
            }
            return CTaskThreadResult();
        }
        // 旧桶已关闭，任何生产者都可帮助摘除，不等待消费者补完删除。
        status = m_buckets.lockFreeMapErase(level, token);
        if (status != LOCK_FREE_MAP_COMMITTED && status != LOCK_FREE_MAP_CONFLICT)
        {
            return CTaskResultAdapter::map(status);
        }
    }
}

LockFreeQueueResult CTaskThreadState::publish(CTaskStorageCache& cache, CTaskPriorityBucket& bucket, CTaskStorageSlot* record)
{
    if (!m_pinCommon || bucket.m_level > 16 || m_cachedBuckets[bucket.m_level - 1].peek() != &bucket)
    {
        return bucket.m_queue.tryPushEntryAndFence(*record);
    }
    LockFreeQueueProducer<CTaskStorageSlot*>& producer = cache.m_producers[bucket.m_level - 1];
    if (!producer.attached())
    {
        LockFreeQueueResult acquired = bucket.m_queue.tryAcquireProducer(producer);
        if (acquired == LockFreeQueueTemporaryUnavailable && m_storage.releaseIdleProducers() != 0)
        {
            acquired = bucket.m_queue.tryAcquireProducer(producer);
        }
        if (acquired != LockFreeQueueSuccess)
        {
            return acquired;
        }
        cache.m_producerCount.store(cache.m_producerCount.load(std::memory_order_relaxed) + 1, std::memory_order_release);
    }
    LockFreeQueueResult result = producer.tryPushEntryAndFence(*record);
    if ((result == LockFreeQueueCapacityLimit || result == LockFreeQueueTemporaryUnavailable) && m_storage.releaseIdleProducers() != 0)
    {
        result = producer.tryPushEntryAndFence(*record);
    }
    return result;
}

CTaskThreadResult CTaskThreadState::post(const std::shared_ptr<CTask>& task, int32_t level)
{
    if (!task || level < 1)
    {
        m_rejected.fetch_add(1, std::memory_order_relaxed);
        return CTaskThreadResult(CTaskThreadInvalid);
    }
    if (!m_accepting.load(std::memory_order_seq_cst))
    {
        m_rejected.fetch_add(1, std::memory_order_relaxed);
        return CTaskThreadResult(CTaskThreadClosed);
    }
    CTaskReadPosition* reader = m_readDomain.enter();
    if (reader == nullptr)
    {
        m_rejected.fetch_add(1, std::memory_order_relaxed);
        return CTaskThreadResult(CTaskThreadRetry);
    }
    // 关闭与登记使用 SC 顺序；关闭方观察不到本登记时，本次必能观察到关闭并拒绝访问池。
    if (!m_accepting.load(std::memory_order_seq_cst))
    {
        m_readDomain.leave(*reader);
        m_rejected.fetch_add(1, std::memory_order_relaxed);
        return CTaskThreadResult(CTaskThreadClosed);
    }
    CTaskStorageCache& cache = m_storage.cache(m_readDomain.indexOf(*reader));
    CTaskCallbackScope scope(this);
    if (!reserveTask(cache))
    {
        m_readDomain.leave(*reader);
        m_rejected.fetch_add(1, std::memory_order_relaxed);
        return CTaskThreadResult(CTaskThreadCapacity);
    }
    std::shared_ptr<CTaskPriorityBucket> owner;
    CTaskPriorityBucket* bucket = nullptr;
    CTaskStorageSlot* record = nullptr;
    bool writer = false;
    CTaskThreadResult result;
    try
    {
        if (m_pinCommon && level <= 16)
        {
            bucket = m_cachedBuckets[level - 1].peek();
            if (bucket != nullptr && bucket->m_writers.closed())
            {
                bucket = nullptr;
            }
        }
        if (bucket == nullptr)
        {
            result = acquireBucket(level, owner);
            writer = result.m_status == CTaskThreadOk;
            bucket = writer ? owner.get() : nullptr;
        }
        if (bucket != nullptr)
        {
            // 发布任务前扩展扫描上界；上界只增不减，不让每次消费和投递争写同一提示。
            if (level <= 16 && m_readyIndex.highest() < level)
            {
                m_readyIndex.publish(level);
            }
            const uint64_t sequence = nextSequence(cache);
            if (sequence == 0)
            {
                result = CTaskThreadResult(CTaskThreadSequenceExhausted);
            }
            else
            {
                record = m_storage.acquire(cache, result, level);
                if (record != nullptr)
                {
                    record->m_task = task;
                    record->m_level = level;
                    record->m_value = record;
                    // 准备身份尚不能被停止遍历观察；独立发布位只在 FIFO 提交后打开。
                    std::atomic<bool>& publication = record->m_page->m_publications[record->index()];
                    countPublication(cache, *bucket, true);
                    const LockFreeQueueResult published = publish(cache, *bucket, record);
                    result = published == LockFreeQueueSuccess ? CTaskThreadResult(CTaskThreadOk, true) : CTaskResultAdapter::queue(published);
                    if (result.m_committed)
                    {
                        // 消费者可能已经执行完毕；只开放可见性，不把执行阶段写回排队。
                        publication.store(true, std::memory_order_release);
                    }
                    else
                    {
                        countPublication(cache, *bucket, false);
                    }
                }
            }
        }
    }
    catch (const std::bad_alloc&)
    {
        result = CTaskThreadResult(CTaskThreadNoMemory);
    }
    catch (...)
    {
        result = CTaskThreadResult(CTaskThreadCallbackFailure);
    }
    if (writer)
    {
        bucket->m_writers.leave();
    }
    if (!result.m_committed)
    {
        if (record != nullptr)
        {
            // 未发布的记录没有任何外部观察者，可以直接归还，不进入退休链。
            m_storage.abandon(*record);
        }
        if (m_config.m_maximumTasks == -1)
        {
            cache.m_reserved.store(cache.m_reserved.load(std::memory_order_relaxed) - 1, std::memory_order_release);
        }
        else
        {
            m_outstanding.fetch_sub(1, std::memory_order_release);
        }
        m_rejected.fetch_add(1, std::memory_order_relaxed);
    }
    else
    {
        // 有序发布接口已经完成队列提交到后续读取之间的完整屏障。
        // 独立发布位仅用于停止遍历，不参与队列非空或唤醒判定。
        wake(true);
        if (level > 1)
        {
            const CTaskThreadResult notice = preempt(level, record);
            if (notice.m_status != CTaskThreadOk)
            {
                result = notice;
                result.m_committed = true;
            }
        }
    }
    m_readDomain.leave(*reader);
    if (!m_accepting.load(std::memory_order_seq_cst))
    {
        wake();
    }
    return result;
}

CTaskThreadStatus CTaskThreadState::notifyStop(const std::shared_ptr<CTask>& task)
{
    if (CTaskCallbackScope::stopping(task.get()))
    {
        return CTaskThreadBusy;
    }
    CTaskCallbackScope scope(this, task.get());
    CTaskThreadStatus status = CTaskThreadOk;
    try
    {
        task->StopTask();
    }
    catch (...)
    {
        m_errors.fetch_add(1, std::memory_order_relaxed);
        status = CTaskThreadCallbackFailure;
    }
    return status;
}

CTaskThreadResult CTaskThreadState::preempt(int32_t level, CTaskStorageSlot* posted)
{
    // 1 是最低合法级别，不可能抢占任何任务；不读取反复更新的当前记录缓存行。
    if (level == 1)
    {
        return CTaskThreadResult();
    }
    const int32_t minimum = m_preemptionFloor.load(std::memory_order_acquire);
    if (level <= minimum)
    {
        return CTaskThreadResult();
    }
    // 已执行的本次投递无需观察任何当前任务；后面仍在取得当前记录后再次验证。
    // 调用者持有读侧保护；唯一消费者自身调用时不并发执行回收。
    CTaskStorageSlot* current = level <= 16 ? m_runningIndex.below(level, minimum) : m_current.load(std::memory_order_acquire);
    if (current == nullptr)
    {
        return CTaskThreadResult();
    }
    if (level <= current->m_level)
    {
        return CTaskThreadResult();
    }
    // 读取当前执行记录之后再检查投递记录；后来的当前记录发布会带来此前执行完成的可见性。
    if (posted && (posted->m_state->m_bits.load(std::memory_order_acquire) & 3) == 3)
    {
        return CTaskThreadResult();
    }
    // 已完成重做决策的记录不再调用业务回调；读侧保护保证此处没有槽位复用。
    if ((current->m_state->m_bits.load(std::memory_order_acquire) & 3) == 3)
    {
        return CTaskThreadResult();
    }
    return requestPreemption(*current);
}

CTaskThreadResult CTaskThreadState::requestPreemption(CTaskStorageSlot& record)
{
    bool redo = false;
    CTaskThreadStatus status = CTaskThreadOk;
    try
    {
        redo = record.m_task->ReExecute();
    }
    catch (...)
    {
        m_errors.fetch_add(1, std::memory_order_relaxed);
        status = CTaskThreadCallbackFailure;
    }
    uint32_t flags = record.m_state->m_bits.load(std::memory_order_acquire);
    for (;;)
    {
        if ((flags & 3) == 3)
        {
            return CTaskThreadResult(status);
        }
        // 不重做时没有需要提交的状态；一次观察已经决定是否发送合作通知。
        if (!redo || record.m_state->m_bits.compare_exchange_weak(flags, flags | 4, std::memory_order_acq_rel, std::memory_order_acquire))
        {
            break;
        }
    }
    const CTaskThreadStatus stopped = notifyStop(record.m_task);
    return CTaskThreadResult(stopped == CTaskThreadOk ? status : stopped);
}

CTaskThreadResult CTaskThreadState::stop(int32_t propertyId, int32_t level, bool all, bool currentOnly)
{
    if (!m_observers.enter())
    {
        return CTaskThreadResult(CTaskThreadClosed);
    }
    CTaskReadPosition* reader = m_readDomain.enter();
    if (reader == nullptr)
    {
        m_observers.leave();
        return CTaskThreadResult(CTaskThreadRetry);
    }
    CTaskCallbackScope scope(this);
    try
    {
        if (currentOnly)
        {
            CTaskStorageSlot* current = m_current.load(std::memory_order_acquire);
            const std::shared_ptr<CTask> task = current != nullptr && (current->m_state->m_bits.load(std::memory_order_acquire) & 3) != 3 ? current->m_task : std::shared_ptr<CTask>();
            m_readDomain.leave(*reader);
            reader = nullptr;
            const CTaskThreadStatus status = task ? notifyStop(task) : CTaskThreadOk;
            m_observers.leave();
            m_completion.trySignal();
            return CTaskThreadResult(status, true);
        }
        std::set<std::shared_ptr<CTask>> selected;
        // 在保护内收集强引用，回调时不继续占用池读侧位置；相同任务对象只通知一次。
        m_storage.forEach([&selected, propertyId, level, all](CTaskStorageSlot& record)
        {
            const int32_t phase = record.m_state->m_bits.load(std::memory_order_acquire) & 3;
            if (phase != 3 && (all || record.m_task->GetPropertyId() == propertyId) &&
                (level == 0 || record.m_level == level))
            {
                selected.insert(record.m_task);
            }
        });
        CTaskStorageSlot* current = m_current.load(std::memory_order_acquire);
        if (current != nullptr && (current->m_state->m_bits.load(std::memory_order_acquire) & 3) != 3 &&
            (all || current->m_task->GetPropertyId() == propertyId))
        {
            selected.insert(current->m_task);
        }
        m_readDomain.leave(*reader);
        reader = nullptr;
        CTaskThreadStatus status = CTaskThreadOk;
        for (std::set<std::shared_ptr<CTask>>::const_iterator it = selected.begin(); it != selected.end(); ++it)
        {
            if (notifyStop(*it) != CTaskThreadOk)
            {
                status = CTaskThreadCallbackFailure;
            }
        }
        m_observers.leave();
        m_completion.trySignal();
        return CTaskThreadResult(status, true);
    }
    catch (const std::bad_alloc&)
    {
        m_errors.fetch_add(1, std::memory_order_relaxed);
        if (reader != nullptr)
        {
            m_readDomain.leave(*reader);
        }
        m_observers.leave();
        return CTaskThreadResult(CTaskThreadNoMemory);
    }
    catch (...)
    {
        m_errors.fetch_add(1, std::memory_order_relaxed);
        if (reader != nullptr)
        {
            m_readDomain.leave(*reader);
        }
        m_observers.leave();
        return CTaskThreadResult(CTaskThreadCallbackFailure);
    }
}

void CTaskThreadState::wake(bool fenced)
{
    if (!fenced)
    {
        CTaskReadyIndex::fence();
    }
    // 已有通知时只读，避免每次投递都在同一缓存行上做写入型 RMW。
    // 与消费者清位后的 SC 屏障配对，禁止两边同时漏看队列发布和通知清除。
    if (m_wakePending.load(std::memory_order_acquire))
    {
        return;
    }
    if (m_wakePending.exchange(true, std::memory_order_acq_rel))
    {
        return;
    }
    const SemaphoreResult result = m_wakeup.trySignal();
    if (result.status != SEMAPHORE_OK)
    {
        m_errors.fetch_add(1, std::memory_order_relaxed);
    }
}

bool CTaskThreadState::selfWait() const
{
    return CTaskCallbackScope::contains(this);
}

CTaskThreadResult CTaskThreadState::requestFinish(bool drain)
{
    m_accepting.store(false, std::memory_order_seq_cst);
    if (drain)
    {
        int32_t expected = 0;
        m_stopMode.compare_exchange_strong(expected, 1, std::memory_order_release, std::memory_order_relaxed);
    }
    else
    {
        m_stopMode.store(2, std::memory_order_release);
    }
    CTaskThreadResult result(CTaskThreadOk, true);
    if (m_stopMode.load(std::memory_order_acquire) == 2 && !m_exited.load(std::memory_order_acquire))
    {
        result = stop(0, 0, true, true);
        if (result.m_status == CTaskThreadClosed)
        {
            result.m_status = CTaskThreadOk;
        }
        result.m_committed = true;
    }
    wake();
    return result;
}

CTaskThreadResult CTaskThreadState::finish(bool drain)
{
    if (selfWait())
    {
        return CTaskThreadResult(CTaskThreadSelfWait);
    }
    if (m_joined.load(std::memory_order_acquire))
    {
        return CTaskThreadResult(CTaskThreadOk, true);
    }
    const CTaskThreadResult requested = requestFinish(drain);
    if (requested.m_status != CTaskThreadOk)
    {
        return requested;
    }
    bool expected = false;
    if (!m_joining.compare_exchange_strong(expected, true, std::memory_order_acq_rel, std::memory_order_acquire))
    {
        return CTaskThreadResult(CTaskThreadBusy);
    }
    // 以下等待属于明确的关闭/join 操作，不在投递或查询的进展路径上。
    m_nativeUsers.close();
    while (m_nativeUsers.active() != 0)
    {
        m_completion.waitResult(m_config.m_idleCheckMilliseconds);
    }
    try
    {
        if (m_thread.joinable())
        {
            m_thread.join();
        }
        m_joined.store(true, std::memory_order_release);
        return CTaskThreadResult(CTaskThreadOk, true);
    }
    catch (const std::system_error& error)
    {
        m_joining.store(false, std::memory_order_release);
        m_errors.fetch_add(1, std::memory_order_relaxed);
        return CTaskThreadResult(CTaskThreadSystemFailure, false, error.code().value());
    }
    catch (...)
    {
        m_joining.store(false, std::memory_order_release);
        m_errors.fetch_add(1, std::memory_order_relaxed);
        return CTaskThreadResult(CTaskThreadSystemFailure);
    }
}

CTaskThreadResult CTaskThreadState::setPriority(CTaskSystemPriority priority)
{
    if (!m_nativeUsers.enter())
    {
        return CTaskThreadResult(CTaskThreadClosed);
    }
    CTaskThreadResult result = CTaskThreadPriority::apply(m_thread, priority);
    m_nativeUsers.leave();
    m_completion.trySignal();
    return result;
}

#ifdef _MSC_VER
__forceinline
#else
inline
#endif
bool CTaskThreadState::take(CTaskPriorityBucket*& bucket, CTaskStorageSlot*& record)
{
    bucket = nullptr;
    if (!m_scanBuckets.load(std::memory_order_acquire))
    {
        for (int32_t i = m_readyIndex.highest() - 1; i >= 0; --i)
        {
            CTaskPriorityBucket* item = m_cachedBuckets[i].peek();
            if (item != nullptr)
            {
                prepareFront(*item);
                if (item->m_redo || item->m_front)
                {
                    bucket = item;
                    break;
                }
            }
        }
    }
    else
    {
        bucket = chooseGeneral();
    }
    if (!bucket)
    {
        return false;
    }
    CTaskStorageSlot*& selected = bucket->m_redo != nullptr ? bucket->m_redo : bucket->m_front;
    record = selected;
    selected = nullptr;
    // 下降立即发布；提高采用窗口最低级别，减少相邻高级和低级选择造成的反复失效。
    const int32_t floor = m_preemptionFloor.load(std::memory_order_relaxed);
    if (bucket->m_level < floor)
    {
        m_preemptionFloor.store(bucket->m_level, std::memory_order_release);
    }
    if (bucket->m_level < m_floorMinimum)
    {
        m_floorMinimum = bucket->m_level;
    }
    if (m_floorUses == 0)
    {
        const int32_t replacement = m_floorMinimum;
        if (replacement != floor)
        {
            m_preemptionFloor.store(replacement, std::memory_order_release);
        }
        m_floorMinimum = INT_MAX;
    }
    m_floorUses = (m_floorUses + 1) & 63;
    // 当前指针就是执行身份；不再写第二份执行标志，也不覆盖并发重做请求。
    m_current.store(record, std::memory_order_release);
    m_runningIndex.publish(*record, bucket->m_level);
    // 单一执行级别同时表示忙闲，发布它的 RMW 承担抢占握手的 StoreLoad 顺序。
    PrefetchTaskAddress(record->m_state, m_writePrefetch);
    m_currentLevel.exchange(bucket->m_level, std::memory_order_acq_rel);
    CTaskReadyIndex::fenceAfterRmw();
    m_dequeuedTasks.store(m_dequeuedTasks.load(std::memory_order_relaxed) + 1, std::memory_order_release);
    if (m_pinCommon && bucket->m_level <= 16)
    {
        bucket->m_taken.store(bucket->m_taken.load(std::memory_order_relaxed) + 1, std::memory_order_release);
    }
    else
    {
        bucket->m_queued.fetch_sub(1, std::memory_order_relaxed);
    }
    return true;
}

#ifdef _MSC_VER
__declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
CTaskPriorityBucket* CTaskThreadState::chooseGeneral()
{
    CTaskPriorityBucket* selected = nullptr;
    // 未缓存的级别才构造遍历回调，避免把冷路径的异常清理扩大到每次任务选择。
    m_buckets.forEach([this, &selected](const int32_t& level, const std::shared_ptr<CTaskPriorityBucket>& item)
    {
        if (!item->m_writers.closed())
        {
            prepareFront(*item);
            if ((item->m_redo || item->m_front) && (!selected || level > selected->m_level))
            {
                selected = item.get();
            }
        }
    });
    return selected;
}

#ifdef _MSC_VER
__forceinline
#else
inline
#endif
void CTaskThreadState::prepareFront(CTaskPriorityBucket& bucket)
{
    if (bucket.m_redo != nullptr || bucket.m_front != nullptr)
    {
        return;
    }
    if (bucket.m_readyHead == bucket.m_readyCount)
    {
        if (!bucket.m_queue.possiblyReady())
        {
            return;
        }
        refillFront(bucket);
    }
    if (bucket.m_readyHead != bucket.m_readyCount)
    {
        bucket.m_front = static_cast<CTaskStorageSlot*>(bucket.m_readyEntries[bucket.m_readyHead++]);
    }
}

#ifdef _MSC_VER
__declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
void CTaskThreadState::refillFront(CTaskPriorityBucket& bucket)
{
    bucket.m_readyHead = 0;
    if (bucket.m_queue.popReadyEntryPrefix(bucket.m_readyEntries, 64, bucket.m_readyCount))
    {
        prefetchFront(bucket);
    }
}

#ifdef _MSC_VER
__forceinline
#else
inline
#endif
void CTaskThreadState::prefetchFront(CTaskPriorityBucket& bucket)
{
    // Queue 已经预取首条 Entry；多级单条批次不重复扫描或再次发出同一地址提示。
    // 单级仍保留原有有限合并，多条批次仍预取所有后继。
    if (bucket.m_readyCount == 1 && m_readyIndex.highest() != 1)
    {
        return;
    }
    // 已取得任务后至多追加四次短探测，减少紧追发布尾部的缓存行争用。
    // 每次只做固定八条提示，不等待指定生产者；新级别出现后立即恢复选择。
    for (size_t pass = 0; pass < 4 && bucket.m_readyCount < 64 &&
        m_readyIndex.highest() == 1 && !m_scanBuckets.load(std::memory_order_acquire); ++pass)
    {
        for (size_t pause = 0; pause < 8; ++pause)
        {
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
            _mm_pause();
#elif defined(__i386__) || defined(__x86_64__)
            __asm__ __volatile__("pause");
#else
            std::atomic_signal_fence(std::memory_order_seq_cst);
#endif
        }
        size_t received = 0;
        bucket.m_queue.popReadyEntryPrefix(bucket.m_readyEntries + bucket.m_readyCount, 64 - bucket.m_readyCount, received);
        bucket.m_readyCount += received;
    }
    // 预读不执行后继任务；下一次选择仍重新检查更高优先级。
    for (size_t i = 0; i < bucket.m_readyCount; ++i)
    {
        PrefetchTaskAddress(bucket.m_readyEntries[i]);
    }
    // 短前缀只发出地址预取，让载荷读取与后面的交接屏障重叠。
    // 长前缀已有足够独立请求，继续提前准备后继记录的状态和任务。
    if (bucket.m_readyCount >= 4)
    {
        for (size_t i = 0; i < bucket.m_readyCount; ++i)
        {
            CTaskStorageSlot* record = static_cast<CTaskStorageSlot*>(bucket.m_readyEntries[i]);
            PrefetchTaskAddress(record->m_state, m_writePrefetch);
            PrefetchTaskAddress(record->m_task.get());
        }
    }
}

CTaskStorageSlot* CTaskThreadState::prepareRedo(const CTaskStorageSlot& record, CTask* rawClone)
{
    CTaskStorageSlot* backup = nullptr;
    try
    {
        if (rawClone == record.m_task.get())
        {
            // 工厂禁止返回已执行的自身；在建立控制块前拒绝，避免重复释放。
            m_errors.fetch_add(1, std::memory_order_relaxed);
            return nullptr;
        }
        if (rawClone != nullptr)
        {
            std::shared_ptr<CTask> clone(rawClone);
            if (!clone->cloneFactory())
            {
                if (!clone->Clone(*record.m_task->cloneFactory()))
                {
                    // 复制失败时临时强引用释放刚生成的任务，不提交半成品。
                    m_errors.fetch_add(1, std::memory_order_relaxed);
                    return nullptr;
                }
            }
            const uint64_t sequence = nextSequence(m_storage.cache(m_readDomain.capacity()));
            if (sequence != 0)
            {
                CTaskThreadResult allocated;
                backup = m_storage.acquire(m_storage.cache(m_readDomain.capacity()), allocated, record.m_level);
                if (backup != nullptr)
                {
                    backup->m_task = clone;
                    backup->m_level = record.m_level;
                }
                else
                {
                    m_errors.fetch_add(1, std::memory_order_relaxed);
                }
            }
            else
            {
                m_errors.fetch_add(1, std::memory_order_relaxed);
            }
        }
    }
    catch (...)
    {
        m_errors.fetch_add(1, std::memory_order_relaxed);
    }
    return backup;
}

#ifdef _MSC_VER
__forceinline
#else
inline
#endif
void CTaskThreadState::preemptPending(CTaskStorageSlot& record)
{
    int32_t higher = 0;
    if (!m_scanBuckets.load(std::memory_order_acquire))
    {
        for (int32_t i = m_readyIndex.highest() - 1; i >= record.m_level; --i)
        {
            CTaskPriorityBucket* item = m_cachedBuckets[i].peek();
            // take 已确认高于本任务的消费者本地前缀及重做槽为空。
            // 期间只有生产者能增加排队项；首槽仍为空时无需重复准备本地前缀。
            if (item != nullptr && item->m_queue.possiblyReady())
            {
                prepareFront(*item);
                if (item->m_redo || item->m_front)
                {
                    higher = item->m_level;
                    break;
                }
            }
        }
    }
    else
    {
        CTaskPriorityBucket* selected = chooseGeneral();
        higher = selected != nullptr ? selected->m_level : 0;
    }
    if (higher > record.m_level)
    {
        requestPreemption(record);
    }
}

void CTaskThreadState::completeRedo(CTaskPriorityBucket& bucket, CTaskStorageSlot& record)
{
    CTaskStorageSlot* backup = nullptr;
    // 一次交换同时关闭通知决定和执行阶段，原子保留此前已经提交的重做意图。
    const uint32_t flags = record.m_state->m_bits.exchange(3, std::memory_order_acq_rel);
    m_currentLevel.store(0, std::memory_order_release);
    if ((flags & 4) != 0 && m_stopMode.load(std::memory_order_acquire) != 2)
    {
        // 每次执行只有消费者提交一次决定；多个生产者的请求合并成同一位。
        try
        {
            CTask* rawClone = (*record.m_task->cloneFactory())();
            if (rawClone != nullptr)
            {
                backup = prepareRedo(record, rawClone);
            }
        }
        catch (...)
        {
            m_errors.fetch_add(1, std::memory_order_relaxed);
        }
    }
    if ((flags & 4) != 0 && backup && m_stopMode.load(std::memory_order_acquire) != 2)
    {
        // 替换原任务的额度；不申请 FIFO 节点，不改变原优先级。
        bucket.m_redo = backup;
        backup->m_page->m_publications[backup->index()].store(true, std::memory_order_release);
        countPublication(m_storage.cache(m_readDomain.capacity()), bucket, true);
    }
    else
    {
        if (backup)
        {
            m_storage.abandon(*backup);
        }
        m_releasedTasks.store(m_releasedTasks.load(std::memory_order_relaxed) + 1, std::memory_order_release);
    }
    m_storage.retire(record);
}

void CTaskThreadState::maintain()
{
    // 先摘除最后完成的记录，再允许 epoch 回收；新读者不能取得已归还地址。
    m_runningIndex.clear();
    m_current.store(nullptr, std::memory_order_release);
    const bool pressure = m_trimBuckets.exchange(false, std::memory_order_acq_rel);
    if (pressure || m_config.m_queueShrink)
    {
        // 缩容需要解除旧段保护；常规消费不反复拆除仍可复用的生产者上下文。
        // 登记槽不足或节点池满时，publish 仍立即回收空闲上下文并重试。
        m_storage.releaseIdleProducers();
    }
    const bool retire = pressure || m_buckets.size() > m_config.m_priorityMap.nodes.initialCapacity;
    m_buckets.forEach([this, retire](const int32_t& level, const std::shared_ptr<CTaskPriorityBucket>& bucket)
    {
        bucket->m_queue.maintain(16);
        if (retire && (!m_pinCommon || level > 16) && bucketQueuedCount(*bucket) == 0 && bucket->m_writers.closeIfEmpty())
        {
            std::shared_ptr<CTaskPriorityBucket> found;
            LockFreeMapToken token;
            if (m_buckets.lockFreeMapGet(level, found, token) == LOCK_FREE_MAP_OK && found == bucket)
            {
                m_buckets.lockFreeMapErase(level, token);
            }
            // 关闭门且无在途生产者、无排队项；只读缓存可继续持有空桶，但不滞留其节点池。
            bucket->m_queue.destroy();
        }
    });
    m_storage.collect(256);
    m_storage.maintain();
    size_t released = 0;
    m_buckets.lockFreeMapMaintain(16, released);
}

void CTaskThreadState::discard()
{
    m_runningIndex.clear();
    m_current.store(nullptr, std::memory_order_release);
    m_buckets.forEach([](const int32_t&, const std::shared_ptr<CTaskPriorityBucket>& bucket)
    {
        bucket->m_queue.clear();
        bucket->m_redo = nullptr;
        bucket->m_front = nullptr;
        bucket->m_readyHead = 0;
        bucket->m_readyCount = 0;
        bucket->m_queued.store(0, std::memory_order_relaxed);
    });
    // 保留累计基数；即使 statistics 与清理并发，也不会把旧归还量减去清零后的基数。
    m_dequeuedTasks.store(m_config.m_maximumTasks == -1 ? m_storage.publishedCount() : m_queued.load(std::memory_order_relaxed), std::memory_order_release);
    m_releasedTasks.store(m_config.m_maximumTasks == -1 ? m_storage.reservedCount() : m_outstanding.load(std::memory_order_relaxed), std::memory_order_release);
    // 已关闭投递和观察接纳，全部在途借用归还；这里满足基础容器的静止释放契约。
    m_storage.clear();
    m_buckets.clearAndFreeMemory();
    for (size_t i = 0; i < 16; ++i)
    {
        m_cachedBuckets[i].reset();
    }
}

#ifdef _MSC_VER
__declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
__attribute__((noinline))
#endif
bool CTaskThreadState::takeAfterWake(CTaskPriorityBucket*& bucket, CTaskStorageSlot*& record)
{
    return take(bucket, record);
}

void CTaskThreadState::run()
{
    CTaskCallbackScope scope(this);
    while (m_started.load(std::memory_order_acquire) == 0)
    {
        m_wakeup.waitResult(m_config.m_idleCheckMilliseconds);
    }
    uint32_t maintenance = 0;
    for (;;)
    {
        const int32_t mode = m_stopMode.load(std::memory_order_acquire);
        if ((mode == 2 || m_started.load(std::memory_order_acquire) == 2) && m_readDomain.active() == 0)
        {
            break;
        }
        CTaskPriorityBucket* bucket = nullptr;
        CTaskStorageSlot* record = nullptr;
        if (mode == 2 || !take(bucket, record))
        {
            if (mode == 1 && m_readDomain.active() == 0 && outstandingCount() == 0)
            {
                break;
            }
            maintain();
            // 清通知后重新检查队列和关闭状态，不依赖生产者再次发来通知。
            m_wakePending.store(false, std::memory_order_release);
            CTaskReadyIndex::fence();
            if (m_stopMode.load(std::memory_order_acquire) != mode ||
                (mode != 0 && m_readDomain.active() == 0 && (mode == 2 || outstandingCount() == 0)))
            {
                continue;
            }
            if (mode == 2 || !takeAfterWake(bucket, record))
            {
                const SemaphoreResult result = m_wakeup.waitResult(m_config.m_idleCheckMilliseconds);
                if (result.status != SEMAPHORE_OK && result.status != SEMAPHORE_TIMEOUT)
                {
                    m_errors.fetch_add(1, std::memory_order_relaxed);
                }
                continue;
            }
        }
        // 两条取出路径汇合到同一执行及回收流程，异常任务也只完成一次记账。
        if (m_scanBuckets.load(std::memory_order_acquire) || m_readyIndex.highest() > record->m_level)
        {
            preemptPending(*record);
        }
        if (m_stopMode.load(std::memory_order_acquire) == 2)
        {
            notifyStop(record->m_task);
        }
        else
        {
            try
            {
                record->m_task->DoTask();
            }
            catch (...)
            {
                m_errors.fetch_add(1, std::memory_order_relaxed);
            }
            m_executed.store(m_executed.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
        }
        if (record->m_task->cloneFactory() != nullptr)
        {
            completeRedo(*bucket, *record);
        }
        else
        {
            record->m_state->m_bits.store(3, std::memory_order_release);
            m_currentLevel.store(0, std::memory_order_release);
            m_releasedTasks.store(m_releasedTasks.load(std::memory_order_relaxed) + 1, std::memory_order_release);
            m_storage.retire(*record);
        }
        // 完成位关闭通知；索引可暂存已完成记录，由 below 过滤。
        // 回收前统一 clear，避免逐任务清空同一生产者观察缓存行。
        if (++maintenance % 512 == 0)
        {
            m_runningIndex.clear();
            m_current.store(nullptr, std::memory_order_release);
            m_storage.collect(1024, true);
            if ((maintenance & 4095) == 0 || m_trimBuckets.load(std::memory_order_acquire))
            {
                maintain();
            }
        }
    }
    m_observers.close();
    while (m_observers.active() != 0)
    {
        m_completion.waitResult(m_config.m_idleCheckMilliseconds);
    }
    m_readDomain.close();
    discard();
    m_exited.store(true, std::memory_order_release);
    m_completion.trySignal();
}

CTaskThreadStatistics CTaskThreadState::statistics() const
{
    CTaskThreadStatistics result;
    result.m_outstanding = outstandingCount();
    result.m_queued = queuedCount();
    result.m_executed = m_executed.load(std::memory_order_relaxed);
    result.m_rejected = m_rejected.load(std::memory_order_relaxed);
    result.m_errors = m_errors.load(std::memory_order_relaxed);
    result.m_currentLevel = m_currentLevel.load(std::memory_order_acquire);
    result.m_accepting = m_accepting.load(std::memory_order_acquire);
    result.m_exited = m_exited.load(std::memory_order_acquire);
    return result;
}

CTaskThreadResult CTaskThreadState::waitInfo(std::map<int32_t, int32_t>& counts)
{
    counts.clear();
    if (!m_observers.enter())
    {
        return CTaskThreadResult(CTaskThreadClosed);
    }
    bool failed = false;
    const size_t visited = m_buckets.forEach([this, &counts, &failed](const int32_t& level, const std::shared_ptr<CTaskPriorityBucket>& bucket)
    {
        try
        {
            const uint64_t count = bucketQueuedCount(*bucket);
            if (count != 0)
            {
                counts[level] = static_cast<int32_t>((std::min)(count, static_cast<uint64_t>(INT_MAX)));
            }
        }
        catch (...)
        {
            failed = true;
        }
    });
    const bool unavailable = visited == 0 && !m_buckets.empty();
    m_observers.leave();
    if (failed || unavailable)
    {
        counts.clear();
    }
    return CTaskThreadResult(failed ? CTaskThreadNoMemory : (unavailable ? CTaskThreadRetry : CTaskThreadOk));
}