#include "CTaskPriorityBucket.h"
#include "CTaskResultAdapter.h"

CTaskPriorityBucket::CTaskPriorityBucket(int32_t level) :
m_level(level),
m_entryOnly(true),
m_queue(false),
m_redo(nullptr),
m_readyHead(0),
m_readyCount(0),
m_queued(0),
m_taken(0)
{

}

CTaskThreadResult CTaskPriorityBucket::initialize(const CTaskThreadConfig& config)
{
    CTaskThreadResult result = configure(config.priorityQueueConfig(m_level));
    return result.m_status == CTaskThreadOk ? CTaskResultAdapter::queue(m_queue.tryInit()) : result;
}

CTaskThreadResult CTaskPriorityBucket::configure(const CTaskQueueConfig& config)
{
    LockFreeQueueResult result = m_queue.configurePool(config.m_queue);
    if (result == LockFreeQueueSuccess)
    {
        result = m_queue.configureEntryOnly(config.m_entryOnly);
    }
    if (result == LockFreeQueueSuccess)
    {
        result = m_queue.configureProducerCounters(config.m_distributedCounters);
    }
    if (result == LockFreeQueueSuccess)
    {
        result = m_queue.configureConsumerYieldInterval(config.m_consumerYieldInterval);
    }
    if (result == LockFreeQueueSuccess)
    {
        result = m_queue.configureCancelableReservations(config.m_cancelableReservations);
    }
    if (result == LockFreeQueueSuccess)
    {
        result = m_queue.setGrowPolicy(config.m_queueGrow);
    }
    if (result == LockFreeQueueSuccess)
    {
        result = m_queue.setShrinkPolicy(config.m_queueShrink);
    }
    if (result == LockFreeQueueSuccess)
    {
        result = m_queue.configureAutomaticMaintenance(config.m_maintenanceInterval, config.m_maintenanceBudget);
    }
    if (result == LockFreeQueueSuccess)
    {
        result = m_queue.configureCapacityWait(config.m_capacityRetries, config.m_capacitySleepMilliseconds);
    }
    if (result == LockFreeQueueSuccess)
    {
        m_entryOnly = config.m_entryOnly;
    }
    return CTaskResultAdapter::queue(result);
}

void CTaskPriorityBucket::refillValuePrefix()
{
    CTaskStorageSlot* records[64];
    m_queue.popReadyPrefix(records, 64, m_readyCount);
    for (size_t i = 0; i < m_readyCount; ++i)
    {
        m_readyEntries[i] = records[i];
    }
}

void CTaskPriorityBucket::reserveQueued()
{
    if (m_queued.fetch_add(1, std::memory_order_relaxed) == 0)
    {
        // 首项尚未发布，调用者的生产登记或单消费者所有权保证此时门仍开放。
        // 即使生产者先于维护的关闭 CAS 退出，未消费的任务仍阻止退役。
        m_writers.enter();
    }
}

void CTaskPriorityBucket::releaseQueued()
{
    if (m_queued.fetch_sub(1, std::memory_order_relaxed) == 1)
    {
        m_writers.leave();
    }
}