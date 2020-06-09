#include "CTask.h"

CTask::CTask() :
m_taskId(1),
m_waitForSend(nullptr)
{

}

CTask::CTask(int32_t taskId) :
m_taskId(taskId),
m_waitForSend(nullptr)
{

}

CTask::~CTask()
{

}

void CTask::StopTask()
{
	return;
}

bool CTask::ReExecute()
{
	return false;
}

CTask* CTask::Clone()
{
	return nullptr;
}

int32_t CTask::GetTaskId()
{
    return m_taskId;
}

void CTask::SetWaitForSendHandle(Semaphore* waitForSend)
{
	m_waitForSend = waitForSend;
}

Semaphore* CTask::GetWaitForSendHandle()
{
	return m_waitForSend;
}