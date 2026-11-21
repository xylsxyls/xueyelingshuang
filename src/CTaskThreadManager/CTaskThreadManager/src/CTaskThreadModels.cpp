#include "CTaskThreadModels.h"
#include "CTaskPriorityBucket.h"

CTaskQueueConfig::CTaskQueueConfig() :
m_maintenanceInterval(64),
m_maintenanceBudget(8),
m_consumerYieldInterval(65536),
m_capacityRetries(4096),
m_capacitySleepMilliseconds(1),
m_distributedCounters(true),
m_entryOnly(true),
m_cancelableReservations(true)
{
    m_queue.maximumProducers = 64;
}

LockFreeQueueResult CTaskQueueConfig::configurePool(const LockFreeQueuePoolConfig& config)
{
    try
    {
        LockFreeQueue<CTaskStorageSlot*> probe(false);
        const LockFreeQueueResult valid = probe.configurePool(config);
        if (valid != LockFreeQueueSuccess)
        {
            return valid;
        }
        LockFreeQueuePoolConfig candidate(config);
        // 先复制可能抛异常的回调，提交时仅作不抛出的交换和标量赋值。
        m_queue.m_onGrowth.swap(candidate.m_onGrowth);
        m_queue.initialCapacity = candidate.initialCapacity;
        m_queue.maximumCapacity = candidate.maximumCapacity;
        m_queue.maximumProducers = candidate.maximumProducers;
        m_queue.producerYieldInterval = candidate.producerYieldInterval;
        m_queue.growThreshold = candidate.growThreshold;
        return LockFreeQueueSuccess;
    }
    catch (...)
    {
        return LockFreeQueueAllocationFailure;
    }
}

LockFreeQueueResult CTaskQueueConfig::setGrowPolicy(const LockFreeQueueGrowPolicy& policy)
{
    try
    {
        LockFreeQueueGrowPolicy(policy).swap(m_queueGrow);
        return LockFreeQueueSuccess;
    }
    catch (...)
    {
        return LockFreeQueueAllocationFailure;
    }
}

LockFreeQueueResult CTaskQueueConfig::setShrinkPolicy(const LockFreeQueueShrinkPolicy& policy)
{
    try
    {
        LockFreeQueueShrinkPolicy(policy).swap(m_queueShrink);
        return LockFreeQueueSuccess;
    }
    catch (...)
    {
        return LockFreeQueueAllocationFailure;
    }
}

LockFreeQueueResult CTaskQueueConfig::configureProducerCounters(bool distributed)
{
    m_distributedCounters = distributed;
    return LockFreeQueueSuccess;
}

LockFreeQueueResult CTaskQueueConfig::configureEntryOnly(bool enabled)
{
    m_entryOnly = enabled;
    return LockFreeQueueSuccess;
}

LockFreeQueueResult CTaskQueueConfig::configureCancelableReservations(bool enabled)
{
    m_cancelableReservations = enabled;
    return LockFreeQueueSuccess;
}

LockFreeQueueResult CTaskQueueConfig::configureAutomaticMaintenance(uint32_t interval, size_t workBudget)
{
    if (interval == 0 || workBudget == 0)
    {
        return LockFreeQueueInvalidConfiguration;
    }
    m_maintenanceInterval = interval;
    m_maintenanceBudget = workBudget;
    return LockFreeQueueSuccess;
}

LockFreeQueueResult CTaskQueueConfig::configureCapacityWait(uint32_t retriesBeforeSleep, uint32_t sleepMilliseconds)
{
    if (sleepMilliseconds == 0)
    {
        return LockFreeQueueInvalidConfiguration;
    }
    m_capacityRetries = retriesBeforeSleep;
    m_capacitySleepMilliseconds = sleepMilliseconds;
    return LockFreeQueueSuccess;
}

LockFreeQueueResult CTaskQueueConfig::configureConsumerYieldInterval(uint32_t operations)
{
    if (operations == 0 || operations > 65536)
    {
        return LockFreeQueueInvalidConfiguration;
    }
    m_consumerYieldInterval = operations;
    return LockFreeQueueSuccess;
}

CTaskThreadConfig::CTaskThreadConfig() :
m_taskBuckets(16384),
m_maximumTasks(-1),
m_idleCheckMilliseconds(32),
m_systemPriority(CTaskPriorityInherited)
{
    m_taskMap.lookupCacheCapacity = 0;
}

CTaskThreadResult CTaskThreadConfig::setPriorityQueueConfig(int32_t level, const CTaskQueueConfig& config)
{
    if (level < 1)
    {
        return CTaskThreadResult(CTaskThreadInvalid);
    }
    try
    {
        CTaskPriorityBucket probe(level);
        CTaskThreadResult result = probe.configure(config);
        if (result.m_status != CTaskThreadOk)
        {
            return result;
        }
        // 先构造不可变配置，再替换单级引用；分配或复制失败不影响任何旧项。
        std::shared_ptr<const CTaskQueueConfig> value = std::make_shared<CTaskQueueConfig>(config);
        m_priorityQueues[level] = value;
        return CTaskThreadResult(CTaskThreadOk, true);
    }
    catch (const std::bad_alloc&)
    {
        return CTaskThreadResult(CTaskThreadNoMemory);
    }
    catch (...)
    {
        return CTaskThreadResult(CTaskThreadPolicyFailure);
    }
}

const CTaskQueueConfig& CTaskThreadConfig::priorityQueueConfig(int32_t level) const
{
    const std::map<int32_t, std::shared_ptr<const CTaskQueueConfig>>::const_iterator it = m_priorityQueues.find(level);
    if (it != m_priorityQueues.end())
    {
        return *it->second;
    }
    return *this;
}

bool CTaskThreadConfig::hasQueueShrinkPolicy() const
{
    if (m_queueShrink)
    {
        return true;
    }
    for (std::map<int32_t, std::shared_ptr<const CTaskQueueConfig>>::const_iterator it = m_priorityQueues.begin(); it != m_priorityQueues.end(); ++it)
    {
        if (it->second->m_queueShrink)
        {
            return true;
        }
    }
    return false;
}

CTaskThreadManagerConfig::CTaskThreadManagerConfig()
{

}

CTaskThreadStatistics::CTaskThreadStatistics() :
m_outstanding(0),
m_queued(0),
m_executed(0),
m_rejected(0),
m_errors(0),
m_currentLevel(0),
m_accepting(false),
m_exited(false)
{

}

const char* taskThreadStatusText(CTaskThreadStatus status)
{
    switch (status)
    {
    case CTaskThreadOk:
    {
        return "成功";
    }
    case CTaskThreadInvalid:
    {
        return "参数非法";
    }
    case CTaskThreadClosed:
    {
        return "已停止接纳任务";
    }
    case CTaskThreadCapacity:
    {
        return "达到容量上限";
    }
    case CTaskThreadRetry:
    {
        return "内部资源暂不可用";
    }
    case CTaskThreadNoMemory:
    {
        return "内存分配失败";
    }
    case CTaskThreadPolicyFailure:
    {
        return "扩缩容策略异常";
    }
    case CTaskThreadCallbackFailure:
    {
        return "任务回调异常";
    }
    case CTaskThreadSystemFailure:
    {
        return "系统调用失败";
    }
    case CTaskThreadUnsupported:
    {
        return "当前平台不支持";
    }
    case CTaskThreadNotFound:
    {
        return "对象不存在";
    }
    case CTaskThreadSelfWait:
    {
        return "不能等待自身或自己的回调";
    }
    case CTaskThreadBusy:
    {
        return "尝试操作发生竞争，本次未提交";
    }
    case CTaskThreadSequenceExhausted:
    {
        return "编号已耗尽";
    }
    case CTaskThreadResourceUnavailable:
    {
        return "资源暂时不能安全复用";
    }
    default:
    {
        return "未知线程错误";
    }
    }
}