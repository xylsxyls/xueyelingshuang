#include "CTaskPriorityBucket.h"
#include "CTaskResultAdapter.h"

CTaskPriorityBucket::CTaskPriorityBucket(int32_t level) :
m_level(level),
m_queue(false),
m_redo(nullptr),
m_front(nullptr),
m_readyHead(0),
m_readyCount(0),
m_queued(0),
m_taken(0),
m_ready(false)
{

}

CTaskThreadResult CTaskPriorityBucket::initialize(const CTaskThreadConfig& config)
{
    m_queue.destroy();
    LockFreeQueueResult result = m_queue.configurePool(config.m_queue);
    if (result == LockFreeQueueSuccess)
    {
        result = m_queue.configureEntryOnly(true);
    }
    if (result == LockFreeQueueSuccess)
    {
        result = m_queue.configureProducerCounters(true);
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
        result = m_queue.tryInit();
    }
    return CTaskResultAdapter::queue(result);
}