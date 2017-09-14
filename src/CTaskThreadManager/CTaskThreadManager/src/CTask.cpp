#include "CTask.h"

CTask::CTask() :
m_taskId(1)
{

}

CTask::CTask(int32_t taskId) :
m_taskId(taskId)
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

void CTask::SetWaitForSendHandle(HANDLE waitForSend)
{
	m_waitForSend = waitForSend;
}

HANDLE CTask::GetWaitForSendHandle()
{
	return m_waitForSend;
}