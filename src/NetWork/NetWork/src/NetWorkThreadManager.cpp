#include "NetWorkThreadManager.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

NetWorkThreadManager::NetWorkThreadManager():
m_workThreadIdCounter(-1),
m_sendThreadId(0),
m_heartThreadId(0)
{

}

NetWorkThreadManager::~NetWorkThreadManager()
{
	int32_t index = -1;
	while (index++ != m_vecWorkThreadId.size() - 1)
	{
		CTaskThreadManager::Instance().Uninit(m_vecWorkThreadId[index]);
	}
	CTaskThreadManager::Instance().Uninit(m_heartThreadId);
	CTaskThreadManager::Instance().Uninit(m_sendThreadId);
}

NetWorkThreadManager& NetWorkThreadManager::instance()
{
	static NetWorkThreadManager netWorkThreadManager;
	return netWorkThreadManager;
}

void NetWorkThreadManager::init(int32_t coreCount)
{
	if (!m_vecWorkThreadId.empty())
	{
		return;
	}
	
	int32_t index = -1;
	//*2
	while (index++ != coreCount - 1)
	{
		m_vecWorkThreadId.push_back(CTaskThreadManager::Instance().Init());
	}
	m_sendThreadId = CTaskThreadManager::Instance().Init();
	m_heartThreadId = CTaskThreadManager::Instance().Init();
}

void NetWorkThreadManager::postWorkTaskToThreadPool(const std::shared_ptr<CTask>& spWorkTask)
{
	while (!CTaskThreadManager::Instance().GetThreadInterface(getWorkThreadId())->TryPostTask(spWorkTask, 1));
}

void NetWorkThreadManager::postSendTaskToThread(uint32_t threadId, const std::shared_ptr<CTask>& spSendTask)
{
	CTaskThreadManager::Instance().GetThreadInterface(threadId)->PostTask(spSendTask, 1);
}

void NetWorkThreadManager::postHeartTaskToThread(const std::shared_ptr<CTask>& spHeartTask)
{
	CTaskThreadManager::Instance().GetThreadInterface(m_heartThreadId)->PostTask(spHeartTask);
}

uint32_t NetWorkThreadManager::getWorkThreadId()
{
	return m_vecWorkThreadId[(++m_workThreadIdCounter) % m_vecWorkThreadId.size()];
}