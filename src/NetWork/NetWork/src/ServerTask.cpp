#include "ServerTask.h"
#include "NetServer.h"
#include "ReceiveNetTask.h"
#include "HeadTask.h"

ServerTask::ServerTask():
m_port(0),
m_netServer(nullptr)
{

}

void ServerTask::DoTask()
{
	if (!m_netServer->initServer(m_port))
	{
		return;
	}

	m_netServer->m_receiveThreadId = CTaskThreadManager::Instance().Init();

	std::shared_ptr<ReceiveNetTask> spReceiveTask(new ReceiveNetTask);
	spReceiveTask->setParam(&m_netServer->m_receiveQueue, &m_netServer->m_receiveSemaphore, m_netServer);
	CTaskThreadManager::Instance().GetThreadInterface(m_netServer->m_receiveThreadId)->PostTask(spReceiveTask);

	m_netServer->head();

	m_netServer->loop();
}

void ServerTask::setParam(int32_t port, NetServer* netServer)
{
	m_port = port;
	m_netServer = netServer;
}