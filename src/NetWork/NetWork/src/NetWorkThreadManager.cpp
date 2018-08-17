#include "NetWorkThreadManager.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

NetWorkThreadManager::NetWorkThreadManager()
{

}

NetWorkThreadManager& NetWorkThreadManager::instance()
{
	static NetWorkThreadManager netWorkThreadManager;
	return netWorkThreadManager;
}

void NetWorkThreadManager::init(int32_t coreCount)
{
	if (!m_vecWorkThreadId.empty() || !m_vecSendThreadId.empty())
	{
		return;
	}
	int32_t index = -1;
	while (index++ != coreCount - 1)
	{
		m_vecWorkThreadId.push_back(CTaskThreadManager::Instance().Init());
		m_vecSendThreadId.push_back(CTaskThreadManager::Instance().Init());
	}
}

void NetWorkThreadManager::postSendTaskToThreadPool(std::shared_ptr<CTask> spSendTask)
{
	while (!CTaskThreadManager::Instance().GetThreadInterface(getSendThreadId())->TryPostTask(spSendTask, 1));
}

void NetWorkThreadManager::postWorkTaskToThreadPool(std::shared_ptr<CTask> spWorkTask)
{
	while (!CTaskThreadManager::Instance().GetThreadInterface(getWorkThreadId())->TryPostTask(spWorkTask, 1));
}

uint32_t NetWorkThreadManager::getSendThreadId()
{
	return m_vecSendThreadId[(++m_sendThreadIdCounter) % m_vecSendThreadId.size()];
}

uint32_t NetWorkThreadManager::getWorkThreadId()
{
	return m_vecWorkThreadId[(++m_workThreadIdCounter) % m_vecWorkThreadId.size()];
}



