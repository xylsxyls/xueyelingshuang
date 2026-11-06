#include "CTaskPoolState.h"
#include "CTaskThreadManager.h"
#include "CTaskPoolControlTask.h"
#include "CTaskResultAdapter.h"
#include <algorithm>
#include <chrono>
#include <limits>

#ifdef _MSC_VER
__declspec(thread) static CTaskPoolState* s_currentTaskPool = nullptr;
#else
static thread_local CTaskPoolState* s_currentTaskPool = nullptr;
#endif

CTaskPoolState::CTaskPoolState(const CTaskThreadPoolConfig& config) :
m_config(config),
m_policy(config.m_policy),
m_controller(0),
m_stop(false),
m_accepting(false),
m_closing(false),
m_closed(false),
m_grows(0),
m_shrinks(0),
m_errors(0),
m_idleSince(0),
m_lastResize(0)
{

}

CTaskPoolState::~CTaskPoolState()
{
    shutdown(false);
}

CTaskThreadResult CTaskPoolState::start()
{
    if (m_config.m_initialThreads == 0 || m_config.m_initialThreads > UINT32_MAX ||
        m_config.m_maximumThreads < -1 || (m_config.m_maximumThreads != -1 &&
        static_cast<uint64_t>(m_config.m_maximumThreads) < m_config.m_initialThreads) ||
        m_config.m_checkMilliseconds == 0 || m_config.m_checkMilliseconds > 60000 ||
        m_config.m_members.operationRecordLimit < 4)
    {
        return CTaskThreadResult(CTaskThreadInvalid);
    }
    if (!m_stop.is_lock_free() || !m_grows.is_lock_free())
    {
        return CTaskThreadResult(CTaskThreadUnsupported);
    }
    CTaskThreadResult result = CTaskResultAdapter::map(m_workers.lockFreeMapConfigure(m_config.m_members));
    if (result.m_status != CTaskThreadOk)
    {
        return result;
    }
    try
    {
        if (!m_policy)
        {
            m_policy.reset(new CTaskPoolPolicy);
        }
        for (size_t i = 0; i < m_config.m_initialThreads; ++i)
        {
            result = addWorker(true);
            if (result.m_status != CTaskThreadOk)
            {
                shutdown(false);
                return result;
            }
        }
        m_lastResize = nowMilliseconds();
        CTaskThreadConfig control;
        control.m_queue.maximumProducers = 4;
        result = CTaskThreadManager::Instance().initialize(control, m_controller);
        if (result.m_status != CTaskThreadOk)
        {
            shutdown(false);
            return result;
        }
        std::shared_ptr<CTaskThread> controller = CTaskThreadManager::Instance().GetThreadInterface(m_controller);
        if (!controller)
        {
            shutdown(false);
            return CTaskThreadResult(CTaskThreadRetry);
        }
        std::shared_ptr<CTask> task(new CTaskPoolControlTask(*this));
        m_accepting.store(true, std::memory_order_release);
        result = controller->PostTask(task);
        if (!result.m_committed)
        {
            shutdown(false);
        }
        return result;
    }
    catch (const std::bad_alloc&)
    {
        shutdown(false);
        return CTaskThreadResult(CTaskThreadNoMemory);
    }
    catch (...)
    {
        shutdown(false);
        return CTaskThreadResult(CTaskThreadSystemFailure);
    }
}

CTaskThreadResult CTaskPoolState::addWorker(bool initial)
{
    uint32_t id = 0;
    CTaskThreadResult result = CTaskThreadManager::Instance().initialize(m_config.m_worker, id);
    if (result.m_status != CTaskThreadOk)
    {
        return result;
    }
    std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(id);
    if (!thread)
    {
        CTaskThreadManager::Instance().Uninit(id);
        return CTaskThreadResult(CTaskThreadRetry);
    }
    try
    {
        m_idleStarts[id] = nowMilliseconds();
    }
    catch (...)
    {
        CTaskThreadManager::Instance().Uninit(id);
        return CTaskThreadResult(CTaskThreadNoMemory);
    }
    result = CTaskResultAdapter::map(m_workers.lockFreeMapSet(id, thread));
    if (!result.m_committed)
    {
        m_idleStarts.erase(id);
        CTaskThreadManager::Instance().Uninit(id);
    }
    else if (!initial)
    {
        m_grows.fetch_add(1, std::memory_order_relaxed);
    }
    return result;
}

uint64_t CTaskPoolState::nowMilliseconds()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

void CTaskPoolState::requestStop()
{
    m_stop.store(true, std::memory_order_release);
    m_changed.trySignal();
}

void CTaskPoolState::run()
{
    s_currentTaskPool = this;
    while (!m_stop.load(std::memory_order_acquire))
    {
        try
        {
            rebalance(nowMilliseconds());
        }
        catch (...)
        {
            m_errors.fetch_add(1, std::memory_order_relaxed);
        }
        m_changed.waitResult(m_config.m_checkMilliseconds);
    }
    m_accepting.store(false, std::memory_order_release);
    s_currentTaskPool = nullptr;
}

void CTaskPoolState::rebalance(uint64_t now)
{
    CTaskPoolLoad load;
    load.m_initialThreads = m_config.m_initialThreads;
    load.m_successfulGrowCount = m_grows.load(std::memory_order_relaxed);
    const size_t observed = m_workers.forEach([this, &load, now](const uint32_t& id, const std::shared_ptr<CTaskThread>& thread)
    {
        const CTaskThreadStatistics state = thread->statistics();
        ++load.m_currentThreads;
        load.m_queuedTasks = state.m_queued > UINT64_MAX - load.m_queuedTasks ? UINT64_MAX : load.m_queuedTasks + state.m_queued;
        load.m_runningThreads += state.m_currentLevel != 0 ? 1 : 0;
        load.m_idleThreads += state.m_outstanding == 0 ? 1 : 0;
        std::map<uint32_t, uint64_t>::iterator idle = m_idleStarts.find(id);
        if (idle != m_idleStarts.end())
        {
            if (state.m_outstanding != 0)
            {
                idle->second = 0;
            }
            else if (idle->second == 0)
            {
                idle->second = now;
            }
        }
    });
    if (observed != m_workers.size())
    {
        m_errors.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    if (load.m_idleThreads == load.m_currentThreads)
    {
        if (m_idleSince == 0)
        {
            m_idleSince = now;
        }
        load.m_idleMilliseconds = now - m_idleSince;
    }
    else
    {
        m_idleSince = 0;
    }
    load.m_sinceResizeMilliseconds = now - m_lastResize;
    const CTaskPoolDecision decision = m_policy->evaluate(load);
    const uint64_t maximum = m_config.m_maximumThreads == -1 ? UINT32_MAX :
        (std::min)(static_cast<uint64_t>(m_config.m_maximumThreads), static_cast<uint64_t>(UINT32_MAX));
    const size_t target = (std::max)(m_config.m_initialThreads,
        static_cast<size_t>((std::min)(static_cast<uint64_t>(decision.m_targetThreads), maximum)));
    size_t current = load.m_currentThreads;
    while (current < target && !m_stop.load(std::memory_order_acquire))
    {
        const CTaskThreadResult result = addWorker(false);
        if (!result.m_committed)
        {
            m_errors.fetch_add(1, std::memory_order_relaxed);
            break;
        }
        ++current;
        m_lastResize = now;
    }
    if (current > target && decision.m_shrinkIdleMilliseconds != 0)
    {
        m_workers.forEach([this, target, &current, now, decision](const uint32_t& id, const std::shared_ptr<CTaskThread>& thread)
        {
            if (current <= target || m_stop.load(std::memory_order_acquire) || thread->statistics().m_outstanding != 0)
            {
                return;
            }
            std::map<uint32_t, uint64_t>::iterator idle = m_idleStarts.find(id);
            if (idle == m_idleStarts.end() || idle->second == 0 || now - idle->second < decision.m_shrinkIdleMilliseconds)
            {
                return;
            }
            // 最终接纳门在 WaitForEnd 中关闭；之前成功投递的任务仍被排空。
            const CTaskThreadResult result = CTaskThreadManager::Instance().WaitForEnd(id);
            if (result.m_status != CTaskThreadOk && result.m_status != CTaskThreadNotFound)
            {
                m_errors.fetch_add(1, std::memory_order_relaxed);
                return;
            }
            std::shared_ptr<CTaskThread> found;
            LockFreeMapToken token;
            if (m_workers.lockFreeMapGet(id, found, token) == LOCK_FREE_MAP_OK && found == thread &&
                m_workers.lockFreeMapErase(id, token) == LOCK_FREE_MAP_COMMITTED)
            {
                --current;
                m_shrinks.fetch_add(1, std::memory_order_relaxed);
                m_lastResize = now;
                m_idleStarts.erase(id);
            }
        });
    }
    size_t released = 0;
    m_workers.lockFreeMapMaintain(64, released);
}

CTaskThreadResult CTaskPoolState::shutdown(bool drain)
{
    if (s_currentTaskPool == this)
    {
        return CTaskThreadResult(CTaskThreadSelfWait);
    }
    if (m_closed.load(std::memory_order_acquire))
    {
        return CTaskThreadResult(CTaskThreadOk, true);
    }
    // 从本池业务任务中同步销毁整个池同样可能等待自身。
    bool self = false;
    const size_t visited = m_workers.forEach([&self](const uint32_t&, const std::shared_ptr<CTaskThread>& thread)
    {
        self = self || thread->selfWait();
    });
    if (self)
    {
        return CTaskThreadResult(CTaskThreadSelfWait);
    }
    if (visited == 0 && !m_workers.empty())
    {
        return CTaskThreadResult(CTaskThreadRetry);
    }
    bool expected = false;
    if (!m_closing.compare_exchange_strong(expected, true, std::memory_order_acq_rel, std::memory_order_acquire))
    {
        return CTaskThreadResult(CTaskThreadBusy);
    }
    m_accepting.store(false, std::memory_order_release);
    requestStop();
    CTaskThreadResult result(CTaskThreadOk, true);
    if (m_controller != 0)
    {
        result = CTaskThreadManager::Instance().WaitForEnd(m_controller);
        if (result.m_status != CTaskThreadOk && result.m_status != CTaskThreadNotFound)
        {
            m_closing.store(false, std::memory_order_release);
            return result;
        }
        m_controller = 0;
    }
    const size_t notified = m_workers.forEach([drain](const uint32_t&, const std::shared_ptr<CTaskThread>& thread)
    {
        thread->requestShutdown(drain);
    });
    bool complete = notified == m_workers.size();
    const size_t joined = m_workers.forEach([&complete, &result](const uint32_t& id, const std::shared_ptr<CTaskThread>&)
    {
        const CTaskThreadResult item = CTaskThreadManager::Instance().WaitForEnd(id);
        if (item.m_status != CTaskThreadOk && item.m_status != CTaskThreadNotFound)
        {
            complete = false;
            result = item;
        }
    });
    if (complete && joined == m_workers.size())
    {
        m_workers.clear();
        size_t released = 0;
        m_workers.lockFreeMapMaintain((std::numeric_limits<size_t>::max)(), released);
        m_closed.store(true, std::memory_order_release);
        result = CTaskThreadResult(CTaskThreadOk, true);
    }
    if (!m_closed.load(std::memory_order_acquire) && result.m_status == CTaskThreadOk)
    {
        result = CTaskThreadResult(CTaskThreadRetry);
    }
    m_closing.store(false, std::memory_order_release);
    return result;
}

CTaskPoolStatistics CTaskPoolState::statistics() const
{
    CTaskPoolStatistics result;
    result.m_threads = m_workers.size();
    result.m_grows = m_grows.load(std::memory_order_relaxed);
    result.m_shrinks = m_shrinks.load(std::memory_order_relaxed);
    result.m_errors = m_errors.load(std::memory_order_relaxed);
    result.m_accepting = m_accepting.load(std::memory_order_acquire);
    return result;
}

CTaskThreadResult CTaskPoolState::threadIds(std::vector<uint32_t>& ids) const
{
    ids.clear();
    bool failed = false;
    const size_t visited = m_workers.forEach([&ids, &failed](const uint32_t& id, const std::shared_ptr<CTaskThread>&)
    {
        try
        {
            ids.push_back(id);
        }
        catch (...)
        {
            failed = true;
        }
    });
    if (failed)
    {
        ids.clear();
    }
    return CTaskThreadResult(failed ? CTaskThreadNoMemory :
        ((visited == 0 && !m_workers.empty()) ? CTaskThreadRetry : CTaskThreadOk));
}

CTaskThreadResult CTaskPoolState::choose(uint64_t turn, uint32_t& id, std::shared_ptr<CTaskThread>& thread)
{
    if (!m_accepting.load(std::memory_order_acquire))
    {
        return CTaskThreadResult(CTaskThreadClosed);
    }
    uint64_t lowest = UINT64_MAX;
    size_t rank = (std::numeric_limits<size_t>::max)();
    size_t position = 0;
    const size_t count = (std::max)(size_t(1), m_workers.size());
    const size_t offset = static_cast<size_t>(turn % count);
    m_workers.forEach([&](const uint32_t& key, const std::shared_ptr<CTaskThread>& candidate)
    {
        const CTaskThreadStatistics state = candidate->statistics();
        const size_t candidateRank = (position++ + offset) % count;
        if (state.m_accepting && (m_config.m_worker.m_maximumTasks == -1 ||
            state.m_outstanding < static_cast<uint64_t>(m_config.m_worker.m_maximumTasks)) &&
            (state.m_outstanding < lowest || (state.m_outstanding == lowest && candidateRank < rank)))
        {
            lowest = state.m_outstanding;
            rank = candidateRank;
            id = key;
            thread = candidate;
        }
    });
    return CTaskThreadResult(thread ? CTaskThreadOk : CTaskThreadRetry, thread != nullptr);
}