#include "SplitViewerLogicTask.h"
#include "SplitViewerWorkTask.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <exception>

SplitViewerLogicTask::SplitViewerLogicTask(const std::shared_ptr<SplitViewerTaskContext>& context,
    const std::shared_ptr<SplitViewerTaskRequest>& request, bool completion) :
m_context(context),
m_request(request),
m_completion(completion),
m_exit(false)
{

}

void SplitViewerLogicTask::StopTask()
{
    m_exit.store(true);
}

void SplitViewerLogicTask::DoTask()
{
    if (m_exit.load() || m_context->m_exit.load())
    {
        return;
    }
    try
    {
        if (!m_completion && m_request->m_longRunning)
        {
            const std::lock_guard<std::mutex> admission(m_context->m_dispatchMutex);
            if (m_context->m_exit.load())
            {
                return;
            }
            const std::shared_ptr<CTaskThread> execution =
                CTaskThreadManager::Instance().GetThreadInterface(m_context->m_executionId);
            if (execution)
            {
                execution->PostTask(std::make_shared<SplitViewerWorkTask>(m_context, m_request));
                return;
            }
        }
        else if (!m_completion)
        {
            m_request->m_success = m_request->m_work(m_exit);
        }
    }
    catch (const std::exception& error)
    {
        m_request->m_success = false;
        LOGERROR("Logic request failed; request=%llu error=%s", m_request->m_id, error.what());
    }
    catch (...)
    {
        m_request->m_success = false;
        LOGERROR("Logic request failed with unknown exception; request=%llu", m_request->m_id);
    }
    if (!m_context->m_exit.load())
    {
        std::atomic_store(&m_context->m_completed, m_request);
    }
}