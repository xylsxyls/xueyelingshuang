#ifndef LOCK_FREE_QUEUE_POLICY_INL
#define LOCK_FREE_QUEUE_POLICY_INL

inline LockFreeQueuePoolConfig::LockFreeQueuePoolConfig() :
initialCapacity(64),
growThreshold(0),
maximumCapacity(-1),
maximumProducers(1024)
{

}

inline LockFreeQueueError::LockFreeQueueError(LockFreeQueueResult result) :
m_result(result)
{

}

inline LockFreeQueueResult LockFreeQueueError::status() const
{
    return m_result;
}

inline const char* LockFreeQueueError::what() const throw()
{
    switch (m_result)
    {
    case LockFreeQueueCapacityLimit:
        return "LockFreeQueue: physical capacity exhausted";
    case LockFreeQueueAllocationFailure:
        return "LockFreeQueue: allocation failed";
    case LockFreeQueueConstructionFailure:
        return "LockFreeQueue: payload construction failed";
    case LockFreeQueueNotInitialized:
        return "LockFreeQueue: not initialized";
    case LockFreeQueueTemporaryUnavailable:
        return "LockFreeQueue: resources temporarily unavailable";
    case LockFreeQueueGrowthDeclined:
        return "LockFreeQueue: growth declined";
    case LockFreeQueuePolicyFailure:
        return "LockFreeQueue: policy failed";
    case LockFreeQueueInvalidConfiguration:
        return "LockFreeQueue: invalid configuration";
    case LockFreeQueueAtomicUnavailable:
        return "LockFreeQueue: required atomics are not lock-free";
    case LockFreeQueueClockUnavailable:
        return "LockFreeQueue: monotonic clock unavailable";
    case LockFreeQueueSequenceExhausted:
        return "LockFreeQueue: structural sequence exhausted; quiescent reset required";
    default:
        return "LockFreeQueue: success";
    }
}

#endif