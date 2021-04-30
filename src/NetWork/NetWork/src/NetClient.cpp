#include "NetClient.h"
#include "HeartTask.h"
#include "WorkTask.h"
#include "NetWorkHelper.h"

NetClient::NetClient() :
m_server(nullptr),
m_receiveThreadId(0),
m_heartThreadId(0),
m_receiveThread(nullptr),
m_heartThread(nullptr)
{

}

void NetClient::connect(const char* ip, int32_t port, bool sendHeart)
{
	initClient(ip, port);
	m_receiveThreadId = CTaskThreadManager::Instance().Init();
	m_receiveThread = CTaskThreadManager::Instance().GetThreadInterface(m_receiveThreadId);
	loop();
	if (sendHeart)
	{
		heart();
	}
}

void NetClient::send(const char* buffer, int32_t length, MessageType type)
{
	if (m_server == nullptr)
	{
		return;
	}
	LibuvTcp::send(m_server, buffer, length, type);
}

void NetClient::receive(uv_tcp_t* sender, const char* buffer, int32_t length)
{
	if (m_server == nullptr)
	{
		return;
	}
	NetWorkHelper::receive(sender, buffer, length, m_receiveArea, m_receiveThread, this);
}

void NetClient::onReceive(const char* buffer, int32_t length, MessageType type)
{
	
}

void NetClient::onServerConnected()
{

}

void NetClient::serverConnected(uv_tcp_t* server)
{
	m_server = server;
	onServerConnected();
}

void NetClient::heart(int32_t time)
{
	if (m_heartThreadId == 0)
	{
		m_heartThreadId = CTaskThreadManager::Instance().Init();
		m_heartThread = CTaskThreadManager::Instance().GetThreadInterface(m_heartThreadId);
	}
	std::shared_ptr<HeartTask> spHeartTask(new HeartTask);
	spHeartTask->setParam(time, this);
	m_heartThread->PostTask(spHeartTask, 1);
}

void NetClient::onHeart()
{

}