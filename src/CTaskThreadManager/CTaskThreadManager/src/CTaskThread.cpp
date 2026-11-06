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

CTaskThreadResult CTaskThread::PostTask(const std::shared_ptr<CTask>& task, int32_t taskLevel)
{
    return m_state->post(task, taskLevel);
}

bool CTaskThread::TryPostTask(const std::shared_ptr<CTask>& task, int32_t taskLevel)
{
    return PostTask(task, taskLevel).m_committed;
}

bool CTaskThread::HasTask()
{
    return statistics().m_queued != 0;
}

CTaskThreadResult CTaskThread::StopCurTask()
{
    return m_state->stop(0, 0, true, true);
}

CTaskThreadResult CTaskThread::StopTask(int32_t propertyId, int32_t taskLevel)
{
    return m_state->stop(propertyId, taskLevel, false, false);
}

CTaskThreadResult CTaskThread::StopAllTask()
{
    return m_state->stop(0, 0, true, false);
}

CTaskThreadResult CTaskThread::GetWaitTaskInfo(std::map<int32_t, int32_t>& counts)
{
    return m_state->waitInfo(counts);
}

int32_t CTaskThread::GetWaitTaskCount()
{
    return static_cast<int32_t>((std::min)(statistics().m_queued, static_cast<uint64_t>(INT_MAX)));
}

int32_t CTaskThread::GetCurTaskLevel()
{
    return statistics().m_currentLevel;
}

CTaskThreadResult CTaskThread::WaitForEnd()
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