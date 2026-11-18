#include "CTaskThread.h"
#include "CTaskThreadState.h"
#include <algorithm>
#include <climits>

CTaskThread::CTaskThread(uint32_t threadId, const CTaskThreadConfig& config) :
m_threadId(threadId),
m_state(new CTaskThreadState(config))
{

}

CTaskThread::~CTaskThread()
{

}

CTaskThreadResult CTaskThread::createThread()
{
    return m_state->start();
}

bool CTaskThread::postTask(const std::shared_ptr<CTask>& task, int32_t taskLevel)
{
    return m_state->post(task, taskLevel, true, true).m_committed;
}

CTaskThreadResult CTaskThread::postTaskResult(const std::shared_ptr<CTask>& task, int32_t taskLevel)
{
    // 普通投递由状态层消化竞争；无需经过仅为try转换Busy的外层入口。
    return m_state->post(task, taskLevel, true, true);
}

bool CTaskThread::tryPostTask(const std::shared_ptr<CTask>& task, int32_t taskLevel,
    bool retryContention, bool retryCapacity)
{
    return m_state->post(task, taskLevel, retryContention, retryCapacity).m_committed;
}

CTaskThreadResult CTaskThread::tryPostTaskResult(const std::shared_ptr<CTask>& task, int32_t taskLevel,
    bool retryContention, bool retryCapacity)
{
    CTaskThreadResult result = m_state->post(task, taskLevel, retryContention, retryCapacity);
    if (!result.m_committed && result.m_status == CTaskThreadRetry)
    {
        result.m_status = CTaskThreadBusy;
    }
    return result;
}

bool CTaskThread::hasTask()
{
    return m_state->queuedCount() != 0;
}

CTaskThreadResult CTaskThread::stopCurTask()
{
    return m_state->stop(0, 0, true, true);
}

CTaskThreadResult CTaskThread::stopTask(int32_t propertyId, int32_t taskLevel)
{
    return m_state->stop(propertyId, taskLevel, false, false);
}

CTaskThreadResult CTaskThread::stopAllTask()
{
    return m_state->stop(0, 0, true, false);
}

CTaskThreadResult CTaskThread::getWaitTaskInfo(std::map<int32_t, int32_t>& counts)
{
    return m_state->waitInfo(counts);
}

int32_t CTaskThread::getWaitTaskCount()
{
    return static_cast<int32_t>((std::min)(m_state->queuedCount(), static_cast<uint64_t>(INT_MAX)));
}

int32_t CTaskThread::getCurTaskLevel()
{
    return m_state->currentLevel();
}

CTaskThreadResult CTaskThread::waitForEnd()
{
    return finish(true);
}

CTaskThreadResult CTaskThread::finish(bool drain)
{
    return m_state->finish(drain);
}

CTaskThreadResult CTaskThread::requestShutdown(bool drain)
{
    return m_state->requestFinish(drain);
}

void CTaskThread::requestStop()
{
    m_state->requestFinish(false);
}

bool CTaskThread::selfWait() const
{
    return m_state->selfWait();
}

CTaskThreadResult CTaskThread::setSystemPriority(CTaskSystemPriority priority)
{
    return m_state->setPriority(priority);
}

CTaskThreadStatistics CTaskThread::statistics() const
{
    return m_state->statistics();
}

uint32_t CTaskThread::threadId() const
{
    return m_threadId;
}