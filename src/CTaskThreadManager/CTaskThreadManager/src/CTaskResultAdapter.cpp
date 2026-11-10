#include "CTaskResultAdapter.h"
#include <chrono>
#include <thread>
#include <system_error>
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <intrin.h>
#endif

CTaskThreadResult CTaskResultAdapter::pause(uint32_t& attempts)
{
    try
    {
        if (++attempts < 16)
        {
            // 短竞争先用处理器提示，不为一次 CAS 失败立即切换线程。
            // 连续失败仍进入调度让步；这里没有限制普通接口的总重试次数。
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
            _mm_pause();
#elif defined(__i386__) || defined(__x86_64__)
            __asm__ __volatile__("pause");
#else
            std::this_thread::yield();
#endif
        }
        else if (attempts < 64)
        {
            std::this_thread::yield();
        }
        else
        {
            attempts = 0;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return CTaskThreadResult();
    }
    catch (const std::system_error& error)
    {
        return CTaskThreadResult(CTaskThreadSystemFailure, false, error.code().value());
    }
    catch (...)
    {
        return CTaskThreadResult(CTaskThreadSystemFailure);
    }
}

CTaskThreadResult CTaskResultAdapter::snapshot(const LockFreeMap<uint32_t, std::shared_ptr<CTaskThread>>& threads,
    std::vector<std::shared_ptr<CTaskThread>>& snapshot)
{
    uint32_t attempts = 0;
    for (;;)
    {
        snapshot.clear();
        bool failed = false;
        const size_t visited = threads.forEach([&snapshot, &failed](const uint32_t&, const std::shared_ptr<CTaskThread>& thread)
        {
            if (failed)
            {
                return;
            }
            try
            {
                snapshot.push_back(thread);
            }
            catch (...)
            {
                failed = true;
            }
        });
        if (failed)
        {
            snapshot.clear();
            return CTaskThreadResult(CTaskThreadNoMemory);
        }
        if (visited != 0 || threads.empty())
        {
            return CTaskThreadResult();
        }
        const CTaskThreadResult result = pause(attempts);
        if (result.m_status != CTaskThreadOk)
        {
            return result;
        }
    }
}

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
    case LockFreeQueueResourceUnavailable:
    {
        return CTaskThreadResult(CTaskThreadResourceUnavailable);
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
    {
        return CTaskThreadResult(CTaskThreadRetry);
    }
    case LOCK_FREE_MAP_CAPACITY:
    case LOCK_FREE_MAP_RESERVED:
    case LOCK_FREE_MAP_RECORD_LIMIT:
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
    case LockFreeHashMapVersionMismatch:
    case LockFreeHashMapCleared:
    {
        status = CTaskThreadRetry;
        break;
    }
    case LockFreeHashMapCapacityLimit:
    case LockFreeHashMapDirectoryFull:
    case LockFreeHashMapProtectionUnavailable:
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