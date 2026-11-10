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
        return "LockFreeQueue: node or metadata capacity exhausted";
    case LockFreeQueueAllocationFailure:
        return "LockFreeQueue: allocation failed";
    case LockFreeQueueConstructionFailure:
        return "LockFreeQueue: payload construction failed";
    case LockFreeQueueNotInitialized:
        return "LockFreeQueue: not initialized";
    case LockFreeQueueBusy:
        return "LockFreeQueue: try operation lost contention";
    case LockFreeQueueResourceUnavailable:
        return "LockFreeQueue: storage is not safely reusable yet";
    case LockFreeQueueInvalidState:
        return "LockFreeQueue: operation is invalid in the current state";
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