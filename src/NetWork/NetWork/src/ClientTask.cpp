#include "ClientTask.h"
#include "NetClient.h"

ClientTask::ClientTask():
m_port(0),
m_isSendHeart(false),
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
	
	m_netClient->loop();
	if (m_isSendHeart)
	{
		m_netClient->heart();
	}
}

void ClientTask::setParam(const std::string& ip, int32_t port, bool isSendHeart, NetClient* netClient)
{
	m_ip = ip;
	m_port = port;
	m_isSendHeart = isSendHeart;
	m_netClient = netClient;
}