#include "CTaskResultAdapter.h"

CTaskThreadResult CTaskResultAdapter::queue(LockFreeQueueResult status)
{
    switch (status)
    {
    case LockFreeQueueSuccess:
    {
        return CTaskThreadResult(CTaskThreadOk, true);
    }
    case LockFreeQueueCapacityLimit:
    {
        return CTaskThreadResult(CTaskThreadCapacity);
    }
    case LockFreeQueueTemporaryUnavailable:
    {
        return CTaskThreadResult(CTaskThreadRetry);
    }
    case LockFreeQueueAllocationFailure:
    {
        return CTaskThreadResult(CTaskThreadNoMemory);
    }
    case LockFreeQueueGrowthDeclined:
    case LockFreeQueuePolicyFailure:
    {
        return CTaskThreadResult(CTaskThreadPolicyFailure);
    }
    case LockFreeQueueAtomicUnavailable:
    {
        return CTaskThreadResult(CTaskThreadUnsupported);
    }
    case LockFreeQueueSequenceExhausted:
    {
        return CTaskThreadResult(CTaskThreadSequenceExhausted);
    }
    case LockFreeQueueConstructionFailure:
    {
        return CTaskThreadResult(CTaskThreadCallbackFailure);
    }
    default:
    {
        return CTaskThreadResult(CTaskThreadInvalid);
    }
    }
}

CTaskThreadResult CTaskResultAdapter::map(LockFreeMapStatus status)
{
    switch (status)
    {
    case LOCK_FREE_MAP_OK:
    case LOCK_FREE_MAP_EXISTS:
    {
        return CTaskThreadResult();
    }
    case LOCK_FREE_MAP_COMMITTED:
    {
        return CTaskThreadResult(CTaskThreadOk, true);
    }
    case LOCK_FREE_MAP_NOT_FOUND:
    {
        return CTaskThreadResult(CTaskThreadNotFound);
    }
    case LOCK_FREE_MAP_CONFLICT:
    case LOCK_FREE_MAP_RESERVED:
    case LOCK_FREE_MAP_RECORD_LIMIT:
    {
        return CTaskThreadResult(CTaskThreadRetry);
    }
    case LOCK_FREE_MAP_CAPACITY:
    {
        return CTaskThreadResult(CTaskThreadCapacity);
    }
    case LOCK_FREE_MAP_ALLOCATION_FAILURE:
    {
        return CTaskThreadResult(CTaskThreadNoMemory);
    }
    case LOCK_FREE_MAP_GROW_DENIED:
    case LOCK_FREE_MAP_POLICY_EXCEPTION:
    {
        return CTaskThreadResult(CTaskThreadPolicyFailure);
    }
    case LOCK_FREE_MAP_USER_EXCEPTION:
    {
        return CTaskThreadResult(CTaskThreadCallbackFailure);
    }
    case LOCK_FREE_MAP_UNSUPPORTED:
    {
        return CTaskThreadResult(CTaskThreadUnsupported);
    }
    case LOCK_FREE_MAP_VERSION_EXHAUSTED:
    {
        return CTaskThreadResult(CTaskThreadSequenceExhausted);
    }
    default:
    {
        return CTaskThreadResult(CTaskThreadInvalid);
    }
    }
}

CTaskThreadResult CTaskResultAdapter::hash(const LockFreeHashMapResult& result)
{
    CTaskThreadStatus status = CTaskThreadInvalid;
    switch (result.m_status)
    {
    case LockFreeHashMapOk:
    case LockFreeHashMapExists:
    {
        status = CTaskThreadOk;
        break;
    }
    case LockFreeHashMapNotFound:
    {
        status = CTaskThreadNotFound;
        break;
    }
    case LockFreeHashMapReservationBusy:
    case LockFreeHashMapProtectionUnavailable:
    case LockFreeHashMapVersionMismatch:
    case LockFreeHashMapCleared:
    {
        status = CTaskThreadRetry;
        break;
    }
    case LockFreeHashMapCapacityLimit:
    case LockFreeHashMapDirectoryFull:
    {
        status = CTaskThreadCapacity;
        break;
    }
    case LockFreeHashMapNoMemory:
    {
        status = CTaskThreadNoMemory;
        break;
    }
    case LockFreeHashMapGrowthDeclined:
    {
        status = CTaskThreadPolicyFailure;
        break;
    }
    case LockFreeHashMapUserException:
    {
        status = CTaskThreadCallbackFailure;
        break;
    }
    case LockFreeHashMapUnsupported:
    {
        status = CTaskThreadUnsupported;
        break;
    }
    case LockFreeHashMapVersionExhausted:
    {
        status = CTaskThreadSequenceExhausted;
        break;
    }
    default:
    {
        break;
    }
    }
    return CTaskThreadResult(status, result.m_committed);
}