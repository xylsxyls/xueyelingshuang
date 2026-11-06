#include "CTaskThreadState.h"
#include "CTaskCallbackScope.h"
#include "CTaskThreadPriority.h"
#include "CTaskResultAdapter.h"
#include <algorithm>
#include <climits>
#include <set>
#include <limits>
#include <system_error>

CTaskThreadState::CTaskThreadState(const CTaskThreadConfig& config) :
m_config(config),
m_buckets(false),
m_scanBuckets(false),
m_records(config.m_taskBuckets, config.m_taskMap),
m_started(0),
m_stopMode(0),
m_joining(false),
m_exited(false),
m_joined(false),
m_sequence(0),
m_current(0),
m_currentLevel(0),
m_outstanding(0),
m_queued(0),
m_releasedTasks(0),
m_dequeuedTasks(0),
m_executed(0),
m_rejected(0),
m_errors(0),
m_wakePending(false),
m_cleanupNeeded(false),
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
    if (!m_posts.supported() || !m_current.is_lock_free() || !m_started.is_lock_free() || !m_joined.is_lock_free())
    {
        return CTaskThreadResult(CTaskThreadUnsupported);
    }
    CTaskThreadResult result = CTaskResultAdapter::map(m_buckets.lockFreeMapConfigure(m_config.m_priorityMap));
    if (result.m_status != CTaskThreadOk)
    {
        return result;
    }
    result = CTaskResultAdapter::hash(LockFreeHashMapResult(m_records.status()));
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

uint64_t CTaskThreadState::nextSequence()
{
    uint64_t value = m_sequence.load(std::memory_order_relaxed);
    while (value != UINT64_MAX)
    {
        if (m_sequence.compare_exchange_weak(value, value + 1, std::memory_order_relaxed, std::memory_order_relaxed))
        {
            return value + 1;
        }
    }
    return 0;
}

bool CTaskThreadState::reserveTask()
{
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
        const uint64_t reserved = m_outstanding.load(std::memory_order_acquire);
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
        const uint64_t queued = m_queued.load(std::memory_order_acquire);
        if (dequeued == m_dequeuedTasks.load(std::memory_order_acquire))
        {
            return queued - dequeued;
        }
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

CTaskThreadResult CTaskThreadState::post(const std::shared_ptr<CTask>& task, int32_t level)
{
    if (!task || level < 1)
    {
        m_rejected.fetch_add(1, std::memory_order_relaxed);
        return CTaskThreadResult(CTaskThreadInvalid);
    }
    if (!m_posts.enter())
    {
        m_rejected.fetch_add(1, std::memory_order_relaxed);
        return CTaskThreadResult(CTaskThreadClosed);
    }
    CTaskCallbackScope scope(this);
    if (!reserveTask())
    {
        m_posts.leave();
        m_rejected.fetch_add(1, std::memory_order_relaxed);
        return CTaskThreadResult(CTaskThreadCapacity);
    }
    std::shared_ptr<CTaskPriorityBucket> bucket;
    std::shared_ptr<CTaskRecord> record;
    bool writer = false;
    bool registered = false;
    CTaskThreadResult result;
    try
    {
        result = acquireBucket(level, bucket);
        writer = result.m_status == CTaskThreadOk;
        if (writer)
        {
            const uint64_t sequence = nextSequence();
            if (sequence == 0)
            {
                result = CTaskThreadResult(CTaskThreadSequenceExhausted);
            }
            else
            {
                record = std::make_shared<CTaskRecord>(task, sequence, level);
                result = CTaskResultAdapter::hash(m_records.trySet(sequence, record));
                registered = result.m_committed;
                if (registered)
                {
                    bucket->m_queued.fetch_add(1, std::memory_order_relaxed);
                    m_queued.fetch_add(1, std::memory_order_acq_rel);
                    result = CTaskResultAdapter::queue(bucket->m_queue.tryPush(record));
                    if (result.m_committed)
                    {
                        int32_t expected = 0;
                        // 消费者可以先执行；绝不能把 Executing/Done 改回 Queued。
                        record->m_phase.compare_exchange_strong(expected, 1, std::memory_order_release, std::memory_order_relaxed);
                    }
                    else
                    {
                        bucket->m_queued.fetch_sub(1, std::memory_order_relaxed);
                        m_queued.fetch_sub(1, std::memory_order_relaxed);
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
        if (registered)
        {
            record->m_phase.store(3, std::memory_order_release);
            eraseRecord(record->m_sequence);
        }
        m_outstanding.fetch_sub(1, std::memory_order_release);
        m_rejected.fetch_add(1, std::memory_order_relaxed);
    }
    else
    {
        wake();
        const CTaskThreadResult notice = preempt(level, record);
        if (notice.m_status != CTaskThreadOk)
        {
            // 载荷已经发布；后续抢占通知失败仍明确保留提交位，调用方不得重发。
            result = notice;
            result.m_committed = true;
        }
    }
    m_posts.leave();
    if (m_posts.closed())
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

CTaskThreadResult CTaskThreadState::preempt(int32_t level, const std::shared_ptr<CTaskRecord>& posted)
{
    // 1 是最低合法级别，不可能抢占任何任务；不读取反复更新的当前记录缓存行。
    if (level == 1 || level <= m_currentLevel.load(std::memory_order_acquire))
    {
        return CTaskThreadResult();
    }
    const uint64_t sequence = m_current.load(std::memory_order_acquire);
    if (sequence == 0)
    {
        return CTaskThreadResult();
    }
    LockFreeHashMap<uint64_t, std::shared_ptr<CTaskRecord>>::Snapshot snapshot;
    const LockFreeHashMapResult found = m_records.find(sequence, snapshot);
    if (found.m_status == LockFreeHashMapNotFound)
    {
        return CTaskThreadResult();
    }
    if (found.m_status != LockFreeHashMapOk)
    {
        m_errors.fetch_add(1, std::memory_order_relaxed);
        return CTaskResultAdapter::hash(found);
    }
    const std::shared_ptr<CTaskRecord> current = *snapshot.m_value;
    if (level <= current->m_level)
    {
        return CTaskThreadResult();
    }
    // 读取当前执行记录之后再检查投递记录；后来的当前记录发布会带来此前执行完成的可见性。
    if (posted && posted->m_phase.load(std::memory_order_acquire) >= 2)
    {
        return CTaskThreadResult();
    }
    CTaskCallbackScope scope(this);
    bool redo = false;
    CTaskThreadStatus status = CTaskThreadOk;
    try
    {
        redo = current->m_task->ReExecute();
    }
    catch (...)
    {
        m_errors.fetch_add(1, std::memory_order_relaxed);
        status = CTaskThreadCallbackFailure;
    }
    uint32_t flags = current->m_flags.load(std::memory_order_acquire);
    for (;;)
    {
        if ((flags & 2) != 0)
        {
            return CTaskThreadResult(status);
        }
        if (current->m_flags.compare_exchange_weak(flags, flags | (redo ? 1 : 0),
            std::memory_order_acq_rel, std::memory_order_acquire))
        {
            break;
        }
    }
    const CTaskThreadStatus stopped = notifyStop(current->m_task);
    return CTaskThreadResult(stopped == CTaskThreadOk ? status : stopped);
}

CTaskThreadResult CTaskThreadState::stop(int32_t propertyId, int32_t level, bool all, bool currentOnly)
{
    if (!m_observers.enter())
    {
        return CTaskThreadResult(CTaskThreadClosed);
    }
    CTaskCallbackScope scope(this);
    if (currentOnly)
    {
        const uint64_t sequence = m_current.load(std::memory_order_acquire);
        std::shared_ptr<CTaskRecord> current;
        CTaskThreadResult result(CTaskThreadOk, true);
        if (sequence != 0)
        {
            // 单任务停止无需快照控制块或去重集合；内存耗尽时仍可合作唤醒当前任务。
            if (m_records.get(sequence, current))
            {
                result.m_status = notifyStop(current->m_task);
            }
            else if (m_current.load(std::memory_order_acquire) != 0)
            {
                result = CTaskThreadResult(CTaskThreadRetry);
            }
        }
        m_observers.leave();
        m_completion.trySignal();
        return result;
    }
    try
    {
        std::set<std::shared_ptr<CTask>> selected;
        if (!currentOnly)
        {
            const LockFreeHashMapResult result = m_records.forEachResult(
                [&selected, propertyId, level, all](const uint64_t&, const std::shared_ptr<CTaskRecord>& record)
            {
                const int32_t phase = record->m_phase.load(std::memory_order_acquire);
                if ((phase == 1 || phase == 2) && (all || record->m_task->GetPropertyId() == propertyId) &&
                    (phase == 2 || level == 0 || record->m_level == level))
                {
                    selected.insert(record->m_task);
                }
            });
            if (result.m_status != LockFreeHashMapOk)
            {
                m_errors.fetch_add(1, std::memory_order_relaxed);
                m_observers.leave();
                return CTaskResultAdapter::hash(result);
            }
        }
        const uint64_t sequence = m_current.load(std::memory_order_acquire);
        LockFreeHashMap<uint64_t, std::shared_ptr<CTaskRecord>>::Snapshot snapshot;
        if (sequence != 0)
        {
            const LockFreeHashMapResult found = m_records.find(sequence, snapshot);
            if (found.m_status != LockFreeHashMapOk && found.m_status != LockFreeHashMapNotFound)
            {
                m_errors.fetch_add(1, std::memory_order_relaxed);
                m_observers.leave();
                return CTaskResultAdapter::hash(found);
            }
            if (found.m_status == LockFreeHashMapOk)
            {
                const std::shared_ptr<CTaskRecord>& current = *snapshot.m_value;
                if (all || currentOnly || current->m_task->GetPropertyId() == propertyId)
                {
                    selected.insert(current->m_task);
                }
            }
        }
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
        m_observers.leave();
        return CTaskThreadResult(CTaskThreadNoMemory);
    }
    catch (...)
    {
        m_errors.fetch_add(1, std::memory_order_relaxed);
        m_observers.leave();
        return CTaskThreadResult(CTaskThreadCallbackFailure);
    }
}

void CTaskThreadState::wake()
{
    // 已有通知时只读，避免每次投递都在同一缓存行上做写入型 RMW。
    // 消费者先清标志再检查队列，发布发生在本次观察之前，不会落在检查之后而漏通知。
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
    m_posts.close();
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

bool CTaskThreadState::take(std::shared_ptr<CTaskPriorityBucket>& bucket, std::shared_ptr<CTaskRecord>& record)
{
    bucket.reset();
    if (!m_scanBuckets.load(std::memory_order_acquire))
    {
        for (int32_t i = 15; i >= 0; --i)
        {
            CTaskPriorityBucket* item = m_cachedBuckets[i].peek();
            if (item != nullptr && !item->m_writers.closed())
            {
                prepareFront(*item);
                if (item->m_redo || item->m_front)
                {
                    m_cachedBuckets[i].get(bucket);
                    break;
                }
            }
        }
    }
    else
    {
        m_buckets.forEach([this, &bucket](const int32_t& level, const std::shared_ptr<CTaskPriorityBucket>& item)
        {
            if (!item->m_writers.closed())
            {
                prepareFront(*item);
                if ((item->m_redo || item->m_front) && (!bucket || level > bucket->m_level))
                {
                    bucket = item;
                }
            }
        });
    }
    if (!bucket)
    {
        return false;
    }
    record.swap(bucket->m_redo ? bucket->m_redo : bucket->m_front);
    record->m_phase.store(2, std::memory_order_release);
    bucket->m_queued.fetch_sub(1, std::memory_order_relaxed);
    m_dequeuedTasks.store(m_dequeuedTasks.load(std::memory_order_relaxed) + 1, std::memory_order_release);
    return true;
}

void CTaskThreadState::prepareFront(CTaskPriorityBucket& bucket)
{
    if (!bucket.m_front && bucket.m_queue.pop(&bucket.m_front))
    {
        int32_t expected = 0;
        bucket.m_front->m_phase.compare_exchange_strong(expected, 1, std::memory_order_release, std::memory_order_relaxed);
    }
}

void CTaskThreadState::execute(const std::shared_ptr<CTaskPriorityBucket>& bucket, const std::shared_ptr<CTaskRecord>& record)
{
    m_currentLevel.store(record->m_level, std::memory_order_relaxed);
    m_current.store(record->m_sequence, std::memory_order_release);
    std::shared_ptr<CTaskRecord> backup;
    try
    {
        CTask* rawClone = record->m_task->Clone();
        if (rawClone == record->m_task.get())
        {
            // 防止错误实现返回 this 后形成第二个控制块并双重释放。
            rawClone = nullptr;
            m_errors.fetch_add(1, std::memory_order_relaxed);
        }
        std::shared_ptr<CTask> clone(rawClone);
        if (clone)
        {
            const uint64_t sequence = nextSequence();
            if (sequence != 0)
            {
                backup = std::make_shared<CTaskRecord>(clone, sequence, record->m_level);
                if (!m_records.trySet(sequence, backup).m_committed)
                {
                    backup.reset();
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
    // 发布当前引用前到达的高优先级投递也必须触发抢占。
    int32_t higher = 0;
    if (!m_scanBuckets.load(std::memory_order_acquire))
    {
        for (int32_t i = 15; i >= record->m_level; --i)
        {
            CTaskPriorityBucket* item = m_cachedBuckets[i].peek();
            if (item != nullptr && !item->m_writers.closed())
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
        m_buckets.forEach([this, &higher](const int32_t& level, const std::shared_ptr<CTaskPriorityBucket>& item)
        {
            prepareFront(*item);
            if ((item->m_redo || item->m_front) && level > higher)
            {
                higher = level;
            }
        });
    }
    if (higher > record->m_level)
    {
        preempt(higher);
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
        m_executed.fetch_add(1, std::memory_order_relaxed);
    }
    const uint32_t flags = record->m_flags.exchange(2, std::memory_order_acq_rel);
    m_current.store(0, std::memory_order_release);
    m_currentLevel.store(0, std::memory_order_relaxed);
    if ((flags & 1) != 0 && backup && m_stopMode.load(std::memory_order_acquire) != 2)
    {
        // 替换原任务的额度；不申请 FIFO 节点，不改变原优先级。
        bucket->m_redo = backup;
        backup->m_phase.store(1, std::memory_order_release);
        bucket->m_queued.fetch_add(1, std::memory_order_relaxed);
        m_queued.fetch_add(1, std::memory_order_acq_rel);
    }
    else
    {
        if (backup)
        {
            backup->m_phase.store(3, std::memory_order_release);
            eraseRecord(backup->m_sequence);
        }
        m_releasedTasks.store(m_releasedTasks.load(std::memory_order_relaxed) + 1, std::memory_order_release);
    }
    record->m_phase.store(3, std::memory_order_release);
    eraseRecord(record->m_sequence);
}

void CTaskThreadState::eraseRecord(uint64_t sequence)
{
    const LockFreeHashMapResult result = m_records.tryErase(sequence);
    if (!result.m_committed && result.m_status != LockFreeHashMapNotFound)
    {
        m_cleanupNeeded.store(true, std::memory_order_release);
    }
}

void CTaskThreadState::maintain()
{
    const bool pressure = m_trimBuckets.exchange(false, std::memory_order_acq_rel);
    const bool retire = pressure || m_buckets.size() > m_config.m_priorityMap.nodes.initialCapacity;
    m_buckets.forEach([this, retire](const int32_t& level, const std::shared_ptr<CTaskPriorityBucket>& bucket)
    {
        bucket->m_queue.maintain(16);
        if (retire && bucket->m_queued.load(std::memory_order_acquire) == 0 && bucket->m_writers.closeIfEmpty())
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
    // 极端保护记录不足时，之前的删除可能未提交；完成记录仍可安全地被清理。
    if (m_cleanupNeeded.exchange(false, std::memory_order_acq_rel))
    {
        const LockFreeHashMapResult result = m_records.forEachResult([this](const uint64_t& key, const std::shared_ptr<CTaskRecord>& record)
        {
            if (record->m_phase.load(std::memory_order_acquire) == 3)
            {
                eraseRecord(key);
            }
        });
        if (result.m_status != LockFreeHashMapOk)
        {
            m_cleanupNeeded.store(true, std::memory_order_release);
        }
    }
    // tryErase 已经摘除完成项；这里只轮转补查一个桶，避免每完成一个任务再扫描一条长积压链。
    m_records.maintain(1);
    size_t released = 0;
    m_buckets.lockFreeMapMaintain(16, released);
}

void CTaskThreadState::discard()
{
    m_buckets.forEach([](const int32_t&, const std::shared_ptr<CTaskPriorityBucket>& bucket)
    {
        bucket->m_queue.clear();
        bucket->m_redo.reset();
        bucket->m_front.reset();
        bucket->m_queued.store(0, std::memory_order_relaxed);
    });
    // 保留累计基数；即使 statistics 与清理并发，也不会把旧归还量减去清零后的基数。
    m_dequeuedTasks.store(m_queued.load(std::memory_order_relaxed), std::memory_order_release);
    m_releasedTasks.store(m_outstanding.load(std::memory_order_relaxed), std::memory_order_release);
    // 已关闭投递和观察接纳，全部在途借用归还；这里满足基础容器的静止释放契约。
    m_records.clearAndFreeMemory();
    m_buckets.clearAndFreeMemory();
    for (size_t i = 0; i < 16; ++i)
    {
        m_cachedBuckets[i].reset();
    }
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
        if ((mode == 2 || m_started.load(std::memory_order_acquire) == 2) && m_posts.active() == 0)
        {
            break;
        }
        std::shared_ptr<CTaskPriorityBucket> bucket;
        std::shared_ptr<CTaskRecord> record;
        if (mode != 2 && take(bucket, record))
        {
            execute(bucket, record);
            if (++maintenance % 64 == 0)
            {
                maintain();
            }
            continue;
        }
        if (mode == 1 && m_posts.active() == 0 && outstandingCount() == 0)
        {
            break;
        }
        maintain();
        // 先重新允许通知，再复查队列，覆盖生产者在上次空观察与登记等待之间发布的窗口。
        m_wakePending.store(false, std::memory_order_release);
        if (m_stopMode.load(std::memory_order_acquire) != mode ||
            (mode != 0 && m_posts.active() == 0 && (mode == 2 || outstandingCount() == 0)))
        {
            continue;
        }
        if (mode != 2 && take(bucket, record))
        {
            execute(bucket, record);
            continue;
        }
        const SemaphoreResult result = m_wakeup.waitResult(m_config.m_idleCheckMilliseconds);
        if (result.status != SEMAPHORE_OK && result.status != SEMAPHORE_TIMEOUT)
        {
            m_errors.fetch_add(1, std::memory_order_relaxed);
        }
    }
    m_observers.close();
    while (m_observers.active() != 0)
    {
        m_completion.waitResult(m_config.m_idleCheckMilliseconds);
    }
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
    result.m_currentLevel = m_currentLevel.load(std::memory_order_relaxed);
    result.m_accepting = !m_posts.closed();
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
    const size_t visited = m_buckets.forEach([&counts, &failed](const int32_t& level, const std::shared_ptr<CTaskPriorityBucket>& bucket)
    {
        try
        {
            const uint64_t count = bucket->m_queued.load(std::memory_order_relaxed);
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