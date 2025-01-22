#include "AssignThreadManager.h"

AssignThreadManager::AssignThreadManager():
m_enterThreadId(0),
m_threadId(0)
{

}

AssignThreadManager& AssignThreadManager::instance()
{
	static AssignThreadManager s_assignThreadManager;
	return s_assignThreadManager;
}

void AssignThreadManager::init(int32_t assignCount)
{
	m_threadId = CTaskThreadManager::Instance().Init();
	m_enterThreadId = CTaskThreadManager::Instance().Init();
	int32_t index = -1;
	while (index++ != assignCount - 1)
	{
		m_vecAssignThreadId.push_back(CTaskThreadManager::Instance().Init());
	}
}

void AssignThreadManager::postAssignTask(const std::vector<std::shared_ptr<CTask>>& vecAssignTask)
{
	int32_t index = -1;
	while (index++ != vecAssignTask.size() - 1)
	{
		if (index == m_vecAssignThreadId.size())
		{
			break;
		}
		CTaskThreadManager::Instance().GetThreadInterface(m_vecAssignThreadId[index])->PostTask(vecAssignTask[index]);
	}
}

void AssignThreadManager::postTask(const std::shared_ptr<CTask>& spTask, int32_t taskLevel)
{
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spTask, taskLevel);
}

void AssignThreadManager::stopAllTask()
{
	CTaskThreadManager::Instance().GetThreadInterface(m_enterThreadId)->StopAllTask();
	int32_t index = -1;
	while (index++ != m_vecAssignThreadId.size() - 1)
	{
		CTaskThreadManager::Instance().GetThreadInterface(m_vecAssignThreadId[index])->StopAllTask();
	}
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->StopAllTask();
}

void AssignThreadManager::uninit()
{
	CTaskThreadManager::Instance().Uninit(m_enterThreadId);
	m_enterThreadId = 0;
	int32_t index = -1;
	while (index++ != m_vecAssignThreadId.size() - 1)
	{
		CTaskThreadManager::Instance().Uninit(m_vecAssignThreadId[index]);
	}
	m_vecAssignThreadId.clear();
	CTaskThreadManager::Instance().Uninit(m_threadId);
	m_threadId = 0;
}