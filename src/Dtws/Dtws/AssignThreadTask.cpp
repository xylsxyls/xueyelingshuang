#include "AssignThreadTask.h"

extern int32_t g_accountCount;

void AssignThreadTask::DoTask()
{
	if (m_vecSpDoTask.size() != g_accountCount)
	{
		m_isInit.signal();
		return;
	}
	int32_t index = -1;
	while (index++ != g_accountCount - 1)
	{
		uint32_t threadId = CTaskThreadManager::Instance().Init();
		m_vecThreadId.push_back(threadId);
		CTaskThreadManager::Instance().GetThreadInterface(threadId)->PostTask(m_vecSpDoTask[index]);
	}

	m_isInit.signal();

	index = -1;
	while (index++ != g_accountCount - 1)
	{
		CTaskThreadManager::Instance().WaitForEnd(m_vecThreadId[index]);
	}
}

void AssignThreadTask::StopTask()
{
	m_isInit.wait();
	int32_t index = -1;
	while (index++ != g_accountCount - 1)
	{
		auto taskThread = CTaskThreadManager::Instance().GetThreadInterface(m_vecThreadId[index]);
		if (taskThread == nullptr)
		{
			continue;
		}
		taskThread->StopAllTask();
	}
}

void AssignThreadTask::setParam(const std::vector<std::shared_ptr<CTask>>& vecSpDoTask)
{
	m_vecSpDoTask = vecSpDoTask;
}