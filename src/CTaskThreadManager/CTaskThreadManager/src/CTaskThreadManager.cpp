#include "CTaskThreadManager.h"
#include "CTaskManagerState.h"

CTaskThreadManager::CTaskThreadManager() :
m_state(nullptr)
{

}

CTaskThreadManager::~CTaskThreadManager()
{
    delete m_state.load(std::memory_order_relaxed);
}

CTaskThreadManager& CTaskThreadManager::Instance()
{
    static CTaskThreadManager s_instance;
    return s_instance;
}

CTaskManagerState* CTaskThreadManager::state()
{
    CTaskManagerState* current = m_state.load(std::memory_order_acquire);
    if (current == nullptr && m_state.is_lock_free())
    {
        try
        {
            std::unique_ptr<CTaskManagerState> candidate(new CTaskManagerState);
            if (m_state.compare_exchange_strong(current, candidate.get(), std::memory_order_acq_rel, std::memory_order_acquire))
            {
                current = candidate.release();
            }
        }
        catch (...)
        {
            return nullptr;
        }
    }
    return current;
}

CTaskThreadResult CTaskThreadManager::configure(const CTaskThreadManagerConfig& config)
{
    CTaskManagerState* current = state();
    return current ? current->configure(config) : CTaskThreadResult(m_state.is_lock_free() ? CTaskThreadNoMemory : CTaskThreadUnsupported);
}

uint32_t CTaskThreadManager::Init()
{
    try
    {
        return Init(CTaskThreadConfig());
    }
    catch (...)
    {
        return 0;
    }
}

uint32_t CTaskThreadManager::Init(const CTaskThreadConfig& config)
{
    uint32_t id = 0;
    initialize(config, id);
    return id;
}

CTaskThreadResult CTaskThreadManager::initialize(const CTaskThreadConfig& config, uint32_t& threadId)
{
    threadId = 0;
    CTaskManagerState* current = state();
    return current ? current->initialize(config, threadId) : CTaskThreadResult(m_state.is_lock_free() ? CTaskThreadNoMemory : CTaskThreadUnsupported);
}

CTaskThreadResult CTaskThreadManager::WaitForEnd(uint32_t threadId)
{
    CTaskManagerState* current = m_state.load(std::memory_order_acquire);
    return current ? current->finish(threadId, true) : CTaskThreadResult(CTaskThreadNotFound);
}

CTaskThreadResult CTaskThreadManager::Uninit(uint32_t threadId)
{
    CTaskManagerState* current = m_state.load(std::memory_order_acquire);
    return current ? current->finish(threadId, false) : CTaskThreadResult(CTaskThreadNotFound);
}

CTaskThreadResult CTaskThreadManager::UninitAll()
{
    CTaskManagerState* current = m_state.load(std::memory_order_acquire);
    return current ? current->shutdown() : CTaskThreadResult(CTaskThreadOk, true);
}

std::shared_ptr<CTaskThread> CTaskThreadManager::GetThreadInterface(uint32_t threadId)
{
    CTaskManagerState* current = m_state.load(std::memory_order_acquire);
    return current ? current->get(threadId) : std::shared_ptr<CTaskThread>();
}

int32_t CTaskThreadManager::Count()
{
    CTaskManagerState* current = m_state.load(std::memory_order_acquire);
    return current ? current->count() : 0;
}