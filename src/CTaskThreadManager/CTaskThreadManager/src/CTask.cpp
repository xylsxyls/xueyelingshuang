#include "CTask.h"

CTask::CTask(int32_t taskId) :
m_taskId(taskId)
{

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