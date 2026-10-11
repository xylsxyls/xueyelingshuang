#include "SplitViewerWorkTask.h"
#include "SplitViewerLogicTask.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <exception>

SplitViewerWorkTask::SplitViewerWorkTask(
    const std::shared_ptr<SplitViewerTaskContext>& context, const std::shared_ptr<SplitViewerTaskRequest>& request) :
m_context(context),
m_request(request),
m_exit(false)
{

}

void SplitViewerWorkTask::StopTask()
{
    m_exit.store(true);
}

void SplitViewerWorkTask::DoTask()
{
    if (m_exit.load() || m_context->m_exit.load())
    {
        return;
    }
    try
    {
        m_request->m_success = m_request->m_work(m_exit);
    }
    catch (const std::exception& error)
    {
        m_request->m_success = false;
        LOGERROR("Execution request failed; request=%llu error=%s", m_request->m_id, error.what());
    }
    catch (...)
    {
        m_request->m_success = false;
        LOGERROR("Execution request failed with unknown exception; request=%llu", m_request->m_id);
    }
    try
    {
        const std::lock_guard<std::mutex> admission(m_context->m_dispatchMutex);
        if (!m_context->m_exit.load())
        {
            const std::shared_ptr<CTaskThread> logic =
                CTaskThreadManager::Instance().GetThreadInterface(m_context->m_logicId);
            if (logic)
            {
                logic->PostTask(std::make_shared<SplitViewerLogicTask>(m_context, m_request, true));
            }
        }
    }
    catch (const std::exception& error)
    {
        LOGERROR("Completion delivery failed; request=%llu error=%s", m_request->m_id, error.what());
        // 最小失败边界保留真实执行结果；原子槽不再分配，也不触碰GUI
        std::atomic_store(&m_context->m_completed, m_request);
    }
}