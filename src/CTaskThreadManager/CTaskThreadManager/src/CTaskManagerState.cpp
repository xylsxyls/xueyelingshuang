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
        return CTaskThreadResult(CTaskThreadInvalid);
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
            // 已建立的系统线程只登记一次；结构竞争重试登记，不反复建线程。
            uint32_t attempts = 0;
            for (;;)
            {
                result = CTaskResultAdapter::map(m_threads.lockFreeMapSet(id, thread));
                if (result.m_committed || result.m_status != CTaskThreadRetry)
                {
                    break;
                }
                result = CTaskResultAdapter::pause(attempts);
                if (result.m_status != CTaskThreadOk)
                {
                    break;
                }
            }
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
    if (m_threadId.load(std::memory_order_relaxed) >= UINT32_MAX)
    {
        return 0;
    }
    // 64位内部计数为32位外部ID留出边界并发余量，不靠CAS重试取得编号。
    const uint64_t previous = m_threadId.fetch_add(1, std::memory_order_relaxed);
    return previous < UINT32_MAX ? static_cast<uint32_t>(previous + 1) : 0;
}

CTaskThreadResult CTaskManagerState::finish(uint32_t threadId, bool drain)
{
    std::shared_ptr<CTaskThread> thread;
    LockFreeMapToken token;
    uint32_t attempts = 0;
    for (;;)
    {
        const LockFreeMapStatus status = m_threads.lockFreeMapGet(threadId, thread, token);
        if (status == LOCK_FREE_MAP_OK)
        {
            break;
        }
        CTaskThreadResult result = CTaskResultAdapter::map(status);
        if (result.m_status != CTaskThreadRetry)
        {
            return result;
        }
        result = CTaskResultAdapter::pause(attempts);
        if (result.m_status != CTaskThreadOk)
        {
            return result;
        }
    }
    CTaskThreadResult result = thread->finish(drain);
    if (result.m_status != CTaskThreadOk)
    {
        return result;
    }
    // join 完成后继续清理登记。删除凭据冲突时重新查询，不要求外部再次注销。
    for (;;)
    {
        std::shared_ptr<CTaskThread> remaining;
        LockFreeMapStatus status = m_threads.lockFreeMapGet(threadId, remaining, token);
        if (status == LOCK_FREE_MAP_NOT_FOUND)
        {
            result = CTaskThreadResult(CTaskThreadOk, true);
            break;
        }
        if (status == LOCK_FREE_MAP_OK)
        {
            if (remaining != thread)
            {
                // 编号不复用；不同对象说明注册状态损坏，不能删除其他对象。
                result = CTaskThreadResult(CTaskThreadInvalid, true);
                break;
            }
            status = m_threads.lockFreeMapErase(threadId, token);
        }
        if (status == LOCK_FREE_MAP_NOT_FOUND)
        {
            // 同一线程已经 join；另一注销者先完成删除也属于清理成功。
            result = CTaskThreadResult(CTaskThreadOk, true);
            break;
        }
        result = CTaskResultAdapter::map(status);
        if (result.m_committed || result.m_status != CTaskThreadRetry)
        {
            result.m_committed = true;
            break;
        }
        result = CTaskResultAdapter::pause(attempts);
        if (result.m_status != CTaskThreadOk)
        {
            result.m_committed = true;
            break;
        }
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
    uint32_t attempts = 0;
    for (;;)
    {
        bool expected = false;
        if (m_bulk.compare_exchange_strong(expected, true, std::memory_order_acq_rel, std::memory_order_acquire))
        {
            break;
        }
        const CTaskThreadResult paused = CTaskResultAdapter::pause(attempts);
        if (paused.m_status != CTaskThreadOk)
        {
            return paused;
        }
    }
    CTaskThreadResult result(CTaskThreadOk, true);
    m_creators.close();
    try
    {
        while (m_creators.active() != 0)
        {
            result = CTaskResultAdapter::pause(attempts);
            if (result.m_status != CTaskThreadOk)
            {
                break;
            }
        }
        while (result.m_status == CTaskThreadOk && !m_threads.empty())
        {
            std::vector<std::shared_ptr<CTaskThread>> threads;
            result = CTaskResultAdapter::snapshot(m_threads, threads);
            if (result.m_status != CTaskThreadOk)
            {
                break;
            }
            // 快照只保留 shared_ptr，不把 Map 读保护带进停止回调、join 或删除重试。
            for (size_t i = 0; i < threads.size(); ++i)
            {
                const CTaskThreadResult item = threads[i]->requestShutdown(false);
                if (item.m_status != CTaskThreadOk)
                {
                    result = item;
                }
            }
            for (size_t i = 0; i < threads.size(); ++i)
            {
                const CTaskThreadResult item = finish(threads[i]->threadId(), false);
                if (item.m_status != CTaskThreadOk && item.m_status != CTaskThreadNotFound)
                {
                    result = item;
                }
            }
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
    if (result.m_status == CTaskThreadOk)
    {
        result.m_committed = true;
    }
    return result;
}

std::shared_ptr<CTaskThread> CTaskManagerState::get(uint32_t threadId)
{
    std::shared_ptr<CTaskThread> thread;
    LockFreeMapToken token;
    uint32_t attempts = 0;
    while (threadId != 0)
    {
        const LockFreeMapStatus status = m_threads.lockFreeMapGet(threadId, thread, token);
        if (status == LOCK_FREE_MAP_OK)
        {
            break;
        }
        thread.reset();
        if (CTaskResultAdapter::map(status).m_status != CTaskThreadRetry ||
            CTaskResultAdapter::pause(attempts).m_status != CTaskThreadOk)
        {
            break;
        }
    }
    return thread;
}

int32_t CTaskManagerState::count()
{
    return static_cast<int32_t>((std::min)(m_threads.size(), static_cast<size_t>(INT_MAX)));
}