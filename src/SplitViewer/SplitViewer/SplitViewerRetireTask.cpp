#include "SplitViewerRetireTask.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

SplitViewerRetireTask::SplitViewerRetireTask(const std::shared_ptr<SplitViewerTaskContext>& context) :
m_context(context),
m_exit(false)
{

}

void SplitViewerRetireTask::StopTask()
{
    m_exit.store(true);
}

void SplitViewerRetireTask::DoTask()
{
    // 停止标志已经在短提交锁内发布，先收敛生产者，再回收执行者
    CTaskThreadManager::Instance().Uninit(m_context->m_logicId);
    CTaskThreadManager::Instance().Uninit(m_context->m_executionId);
    std::atomic_store(&m_context->m_completed, std::shared_ptr<SplitViewerTaskRequest>());
    m_context->m_retired.store(true);
}