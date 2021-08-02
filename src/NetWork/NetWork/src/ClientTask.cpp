#include "ClientTask.h"
#include "NetClient.h"
#include "ReceiveTask.h"

ClientTask::ClientTask():
m_port(0),
m_netClient(nullptr)
{

}

void ClientTask::DoTask()
{
	if (!m_netClient->initClient(m_ip.c_str(), m_port))
	{
		return;
	}

	m_netClient->m_receiveThreadId = CTaskThreadManager::Instance().Init();

	std::shared_ptr<ReceiveTask> spReceiveTask(new ReceiveTask);
	spReceiveTask->setParam(&m_netClient->m_receiveQueue, &m_netClient->m_receiveSemaphore, m_netClient);
	CTaskThreadManager::Instance().GetThreadInterface(m_netClient->m_receiveThreadId)->PostTask(spReceiveTask);
	
	m_netClient->loop();
}

void ClientTask::setParam(const std::string& ip, int32_t port, NetClient* netClient)
{
	m_ip = ip;
	m_port = port;
	m_netClient = netClient;
}