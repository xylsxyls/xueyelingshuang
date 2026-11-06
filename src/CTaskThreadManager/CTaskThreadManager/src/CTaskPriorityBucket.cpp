#include "CTaskPriorityBucket.h"
#include "CTaskResultAdapter.h"

CTaskPriorityBucket::CTaskPriorityBucket(int32_t level) :
m_level(level),
m_queue(false),
m_queued(0)
{

}

CTaskThreadResult CTaskPriorityBucket::initialize(const CTaskThreadConfig& config)
{
    m_queue.destroy();
    LockFreeQueueResult result = m_queue.configurePool(config.m_queue);
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