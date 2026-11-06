#include "CTaskManagerState.h"
#include "CTaskCallbackScope.h"
#include "CTaskResultAdapter.h"
#include "Semaphore/SemaphoreAPI.h"
#include <algorithm>
#include <climits>
#include <limits>

CTaskManagerState::CTaskManagerState() :
m_bulk(false),
m_threadId(0)
{

}

CTaskManagerState::~CTaskManagerState()
{
    shutdown();
}

CTaskThreadResult CTaskManagerState::configure(const CTaskThreadManagerConfig& config)
{
    if (!m_threads.empty() || m_creators.active() != 0 || m_bulk.load(std::memory_order_acquire))
    {
        return CTaskThreadResult(CTaskThreadBusy);
    }
    if (config.m_threadMap.operationRecordLimit < 4)
    {
        return CTaskThreadResult(CTaskThreadInvalid);
    }
    return CTaskResultAdapter::map(m_threads.lockFreeMapConfigure(config.m_threadMap));
}

CTaskThreadResult CTaskManagerState::initialize(const CTaskThreadConfig& config, uint32_t& threadId)
{
    threadId = 0;
    if (!m_creators.supported())
    {
        return CTaskThreadResult(CTaskThreadUnsupported);
    }
    if (!m_creators.enter())
    {
        return CTaskThreadResult(CTaskThreadClosed);
    }
    CTaskCallbackScope scope(this);
    const uint32_t id = nextThreadId();
    if (id == 0)
    {
        m_creators.leave();
        return CTaskThreadResult(CTaskThreadSequenceExhausted);
    }
    CTaskThreadResult result;
    std::shared_ptr<CTaskThread> thread;
    try
    {
        thread.reset(new CTaskThread(id, config));
        result = thread->createThread();
        if (result.m_status == CTaskThreadOk)
        {
            LockFreeMapStatus status;
            do
            {
                status = m_threads.lockFreeMapSet(id, thread);
            } while (status == LOCK_FREE_MAP_CONFLICT);
            result = CTaskResultAdapter::map(status);
            if (result.m_committed)
            {
                threadId = id;
            }
            else
            {
                thread->finish(false);
            }
        }
    }
    catch (const std::bad_alloc&)
    {
        result = CTaskThreadResult(CTaskThreadNoMemory);
    }
    catch (...)
    {
        result = CTaskThreadResult(CTaskThreadSystemFailure);
    }
    m_creators.leave();
    return result;
}

uint32_t CTaskManagerState::nextThreadId()
{
    uint32_t id = m_threadId.load(std::memory_order_relaxed);
    while (id != UINT32_MAX)
    {
        if (m_threadId.compare_exchange_weak(id, id + 1, std::memory_order_relaxed, std::memory_order_relaxed))
        {
            return id + 1;
        }
    }
    return 0;
}

CTaskThreadResult CTaskManagerState::finish(uint32_t threadId, bool drain)
{
    std::shared_ptr<CTaskThread> thread;
    LockFreeMapToken token;
    const LockFreeMapStatus status = m_threads.lockFreeMapGet(threadId, thread, token);
    if (status != LOCK_FREE_MAP_OK)
    {
        return CTaskResultAdapter::map(status);
    }
    CTaskThreadResult result = thread->finish(drain);
    if (result.m_status != CTaskThreadOk)
    {
        return result;
    }
    const LockFreeMapStatus removed = m_threads.lockFreeMapErase(threadId, token);
    if (removed != LOCK_FREE_MAP_COMMITTED && removed != LOCK_FREE_MAP_CONFLICT)
    {
        result = CTaskResultAdapter::map(removed);
        result.m_committed = true;
    }
    size_t released = 0;
    m_threads.lockFreeMapMaintain((std::numeric_limits<size_t>::max)(), released);
    return result;
}

CTaskThreadResult CTaskManagerState::shutdown()
{
    if (CTaskCallbackScope::active())
    {
        return CTaskThreadResult(CTaskThreadSelfWait);
    }
    bool self = false;
    const size_t visited = m_threads.forEach([&self](const uint32_t&, const std::shared_ptr<CTaskThread>& thread)
    {
        self = self || thread->selfWait();
    });
    if (self)
    {
        return CTaskThreadResult(CTaskThreadSelfWait);
    }
    if (visited == 0 && !m_threads.empty())
    {
        return CTaskThreadResult(CTaskThreadRetry);
    }
    bool expected = false;
    if (!m_bulk.compare_exchange_strong(expected, true, std::memory_order_acq_rel, std::memory_order_acquire))
    {
        return CTaskThreadResult(CTaskThreadBusy);
    }
    CTaskThreadResult result(CTaskThreadOk, true);
    m_creators.close();
    try
    {
        Semaphore pause;
        while (m_creators.active() != 0)
        {
            pause.waitResult(10);
        }
        m_threads.forEach([](const uint32_t&, const std::shared_ptr<CTaskThread>& thread)
        {
            thread->requestStop();
        });
        m_threads.forEach([this, &result](const uint32_t& id, const std::shared_ptr<CTaskThread>&)
        {
            const CTaskThreadResult item = finish(id, false);
            if (item.m_status != CTaskThreadOk && item.m_status != CTaskThreadNotFound)
            {
                result = item;
            }
        });
        if (!m_threads.empty() && result.m_status == CTaskThreadOk)
        {
            result = CTaskThreadResult(CTaskThreadRetry);
        }
        size_t released = 0;
        m_threads.lockFreeMapMaintain((std::numeric_limits<size_t>::max)(), released);
    }
    catch (const std::bad_alloc&)
    {
        result = CTaskThreadResult(CTaskThreadNoMemory);
    }
    catch (...)
    {
        result = CTaskThreadResult(CTaskThreadSystemFailure);
    }
    m_creators.reopen();
    m_bulk.store(false, std::memory_order_release);
    return result;
}

std::shared_ptr<CTaskThread> CTaskManagerState::get(uint32_t threadId)
{
    std::shared_ptr<CTaskThread> thread;
    LockFreeMapToken token;
    if (threadId != 0)
    {
        m_threads.lockFreeMapGet(threadId, thread, token);
    }
    return thread;
}

int32_t CTaskManagerState::count()
{
    return static_cast<int32_t>((std::min)(m_threads.size(), static_cast<size_t>(INT_MAX)));
}