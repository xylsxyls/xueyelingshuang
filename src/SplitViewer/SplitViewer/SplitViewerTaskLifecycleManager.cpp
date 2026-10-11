#include "SplitViewerTaskLifecycleManager.h"
#include "SplitViewerRetireTask.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <exception>

SplitViewerTaskLifecycleManager::SplitViewerTaskLifecycleManager() :
m_retireId(0)
{

}

SplitViewerTaskLifecycleManager& SplitViewerTaskLifecycleManager::instance()
{
    static SplitViewerTaskLifecycleManager s_manager;
    return s_manager;
}

bool SplitViewerTaskLifecycleManager::init()
{
    if (!m_retireId)
    {
        try
        {
            m_retireId = CTaskThreadManager::Instance().Init();
        }
        catch (const std::exception& error)
        {
            LOGERROR("Task coordinator initialization failed; error=%s", error.what());
            return false;
        }
    }
    return m_retireId != 0;
}

void SplitViewerTaskLifecycleManager::retire(const std::shared_ptr<SplitViewerTaskContext>& context)
{
    {
        const std::lock_guard<std::mutex> admission(context->m_dispatchMutex);
        context->m_exit.store(true);
    }
    if (!context->m_logicId && !context->m_executionId)
    {
        context->m_retired.store(true);
        return;
    }
    const std::shared_ptr<CTaskThread> coordinator = CTaskThreadManager::Instance().GetThreadInterface(m_retireId);
    if (coordinator)
    {
        coordinator->PostTask(std::make_shared<SplitViewerRetireTask>(context));
    }
}

void SplitViewerTaskLifecycleManager::finish()
{
    if (m_retireId)
    {
        CTaskThreadManager::Instance().WaitForEnd(m_retireId);
        m_retireId = 0;
    }
}