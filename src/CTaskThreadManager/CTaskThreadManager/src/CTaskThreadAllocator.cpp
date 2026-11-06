#include "CTaskThreadAllocator.h"
#include "CTaskThreadPool.h"

CTaskThreadAllocator::CTaskThreadAllocator(CTaskThreadPool& pool) :
m_pool(&pool),
m_turn(0)
{

}

CTaskThreadResult CTaskThreadAllocator::select(uint32_t& threadId)
{
    std::shared_ptr<CTaskThread> thread;
    return select(threadId, thread);
}

CTaskThreadResult CTaskThreadAllocator::select(uint32_t& threadId, std::shared_ptr<CTaskThread>& thread)
{
    if (!m_turn.is_lock_free())
    {
        threadId = 0;
        thread.reset();
        return CTaskThreadResult(CTaskThreadUnsupported);
    }
    return m_pool->choose(m_turn.fetch_add(1, std::memory_order_relaxed), threadId, thread);
}