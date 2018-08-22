#include "NetWorkThreadManager.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

std::atomic<int> netCalc = 0;

NetWorkThreadManager::NetWorkThreadManager():
m_sendThreadIdCounter(-1),
m_workThreadIdCounter(-1)
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
		m_vecSendThreadId.push_back(CTaskThreadManager::Instance().Init());
		m_vecWorkThreadId.push_back(CTaskThreadManager::Instance().Init());
	}
}

void NetWorkThreadManager::postSendTaskToThreadPool(uint32_t threadId, const std::shared_ptr<CTask>& spSendTask, int32_t taskLevel)
{
	//++netCalc;
	//if (netCalc % 200000 == 0)
	//{
	//	printf("netCalc = %d\n", netCalc);
	//}
	//CTaskThreadManager::Instance().GetThreadInterface(getSendThreadId())->PostTask(spSendTask, 1);
	//while (!CTaskThreadManager::Instance().GetThreadInterface(giveSendThreadId())->TryPostTask(spSendTask, taskLevel));
	CTaskThreadManager::Instance().GetThreadInterface(threadId)->PostTask(spSendTask, taskLevel);
}

void NetWorkThreadManager::postWorkTaskToThreadPool(const std::shared_ptr<CTask>& spWorkTask)
{
	while (!CTaskThreadManager::Instance().GetThreadInterface(getWorkThreadId())->TryPostTask(spWorkTask, 1));
}

uint32_t NetWorkThreadManager::giveSendThreadId()
{
	return m_vecSendThreadId[(++m_sendThreadIdCounter) % m_vecSendThreadId.size()];
	return m_vecSendThreadId[0];
}

uint32_t NetWorkThreadManager::getWorkThreadId()
{
	return m_vecWorkThreadId[(++m_workThreadIdCounter) % m_vecWorkThreadId.size()];
}



