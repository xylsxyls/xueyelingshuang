#include "ClientTask.h"
#include "NetClient.h"
#include "ReceiveNetTask.h"

ClientTask::ClientTask():
m_waitTime(0),
m_port(0),
m_netClient(nullptr)
{

}

void ClientTask::DoTask()
{
	if ((m_waitTime != 0) && m_wait.wait(m_waitTime))
	{
		return;
	}

	if (!m_netClient->initClient(m_ip.c_str(), m_port))
	{
		return;
	}

	m_netClient->m_receiveThreadId = CTaskThreadManager::Instance().Init();

	std::shared_ptr<ReceiveNetTask> spReceiveTask(new ReceiveNetTask);
	spReceiveTask->setParam(&m_netClient->m_receiveQueue, &m_netClient->m_receiveSemaphore, m_netClient);
	CTaskThreadManager::Instance().GetThreadInterface(m_netClient->m_receiveThreadId)->PostTask(spReceiveTask);
	
	m_netClient->loop();
}

void ClientTask::StopTask()
{
	m_wait.signal();
}

void ClientTask::setParam(int32_t waitTime, const std::string& ip, int32_t port, NetClient* netClient)
{
	m_waitTime = waitTime;
	m_ip = ip;
	m_port = port;
	m_netClient = netClient;
}