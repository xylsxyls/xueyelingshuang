#include "AssignThreadTask.h"
#include "Config.h"

AssignThreadTask::AssignThreadTask():
m_exit(false)
{
	int32_t index = -1;
	while (index++ != g_config.m_accountCount - 1)
	{
		uint32_t threadId = CTaskThreadManager::Instance().Init();
		m_vecThreadId.push_back(threadId);
	}
}


AssignThreadTask::~AssignThreadTask()
{
	int32_t index = -1;
	while (index++ != g_config.m_accountCount - 1)
	{
		CTaskThreadManager::Instance().Uninit(m_vecThreadId[index]);
	}
	m_vecThreadId.clear();
}

void AssignThreadTask::DoTask()
{
	if (m_vecSpDoTask.size() != g_config.m_accountCount)
	{
		return;
	}
	{
		std::unique_lock<std::mutex> lock(m_isInit);
		if (m_exit)
		{
			return;
		}
		int32_t index = -1;
		while (index++ != g_config.m_accountCount - 1)
		{
			CTaskThreadManager::Instance().GetThreadInterface(m_vecThreadId[index])->PostTask(m_vecSpDoTask[index]);
		}
	}

	int32_t index = -1;
	while (index++ != g_config.m_accountCount - 1)
	{
		CTaskThreadManager::Instance().WaitForEnd(m_vecThreadId[index]);
	}
}

void AssignThreadTask::StopTask()
{
	std::unique_lock<std::mutex> lock(m_isInit);
	int32_t index = -1;
	while (index++ != g_config.m_accountCount - 1)
	{
		auto taskThread = CTaskThreadManager::Instance().GetThreadInterface(m_vecThreadId[index]);
		if (taskThread != nullptr)
		{
			taskThread->StopAllTask();
		}
	}
	m_exit = true;
}

void AssignThreadTask::setParam(const std::vector<std::shared_ptr<CTask>>& vecSpDoTask)
{
	m_vecSpDoTask = vecSpDoTask;
}