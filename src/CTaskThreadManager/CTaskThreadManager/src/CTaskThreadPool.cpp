#include "CTaskThreadPool.h"
#include "CTaskPoolState.h"

CTaskThreadPool::CTaskThreadPool()
{

}

CTaskThreadPool::~CTaskThreadPool()
{
    shutdown(true);
}

CTaskThreadResult CTaskThreadPool::initialize(const CTaskThreadPoolConfig& config)
{
    if (m_state && m_state->statistics().m_accepting)
    {
        return CTaskThreadResult(CTaskThreadBusy);
    }
    if (m_state)
    {
        CTaskThreadResult result = m_state->shutdown(true);
        if (result.m_status != CTaskThreadOk)
        {
            return result;
        }
    }
    try
    {
        std::unique_ptr<CTaskPoolState> state(new CTaskPoolState(config));
        const CTaskThreadResult result = state->start();
        if (result.m_status == CTaskThreadOk)
        {
            m_state.swap(state);
        }
        return result;
    }
    catch (const std::bad_alloc&)
    {
        return CTaskThreadResult(CTaskThreadNoMemory);
    }
    catch (...)
    {
        return CTaskThreadResult(CTaskThreadSystemFailure);
    }
}

CTaskThreadResult CTaskThreadPool::shutdown(bool drain)
{
    return m_state ? m_state->shutdown(drain) : CTaskThreadResult(CTaskThreadOk, true);
}

CTaskPoolStatistics CTaskThreadPool::statistics() const
{
    return m_state ? m_state->statistics() : CTaskPoolStatistics();
}

CTaskThreadResult CTaskThreadPool::threadIds(std::vector<uint32_t>& ids) const
{
    ids.clear();
    return m_state ? m_state->threadIds(ids) : CTaskThreadResult(CTaskThreadClosed);
}

CTaskThreadResult CTaskThreadPool::choose(uint64_t turn, uint32_t& id, std::shared_ptr<CTaskThread>& thread)
{
    id = 0;
    thread.reset();
    return m_state ? m_state->choose(turn, id, thread) : CTaskThreadResult(CTaskThreadClosed);
}