#include "ServerTask.h"
#include "NetServer.h"

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
	m_netServer->loop();
}

void ServerTask::setParam(int32_t port, NetServer* netServer)
{
	m_port = port;
	m_netServer = netServer;
}