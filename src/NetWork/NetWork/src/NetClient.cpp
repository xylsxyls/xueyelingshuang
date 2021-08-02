#include "NetClient.h"
#include "HeartTask.h"
#include "ReceiveTask.h"
#include "NetWorkHelper.h"
#include "RunLoopTask.h"
#include "ClientTask.h"

NetClient::NetClient() :
m_destServer(nullptr),
m_isConnected(false),
m_receiveThreadId(0),
m_heartThreadId(0),
m_loopThreadId(0),
m_firstMessageLength(0),
m_isSendHeart(false)
{
	m_head[0] = 0;
	m_head[1] = 0;
}

NetClient::~NetClient()
{
	
}

void NetClient::setFirstMessageLength(int32_t length)
{
	m_firstMessageLength = length;
}

void NetClient::connect(const char* ip, int32_t port, bool isSendHeart)
{
	if (m_isConnected)
	{
		return;
	}

	m_isSendHeart = isSendHeart;
	
	m_loopThreadId = CTaskThreadManager::Instance().Init();

	std::shared_ptr<ClientTask> spClientTask(new ClientTask);
	spClientTask->setParam(ip, port, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_loopThreadId)->PostTask(spClientTask);
	
	m_isConnected = true;
}

void NetClient::close()
{
	if (m_heartThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_heartThreadId);
		m_heartThreadId = 0;
	}

	if (m_receiveThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_receiveThreadId);
		m_receiveThreadId = 0;
	}

	if (m_loopThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_loopThreadId);
		m_loopThreadId = 0;
	}

	m_isSendHeart = false;
	m_isConnected = false;
}

void NetClient::sendFirstMessage(const char* buffer, int32_t length)
{
	if (m_destServer == nullptr || (buffer == nullptr && length != 0))
	{
		return;
	}
	LibuvTcp::send(NetWorkHelper::send(this, m_destServer, buffer, length, MessageType::FIRST_MESSAGE));
}

void NetClient::sendFirstMessage(const std::string& message)
{
	sendFirstMessage(message.c_str(), message.size());
}

void NetClient::send(const char* buffer, int32_t length, MessageType type)
{
	if (m_destServer == nullptr || (buffer == nullptr && length != 0) || m_head[0] == 0)
	{
		return;
	}
	LibuvTcp::send(NetWorkHelper::send(this, m_destServer, buffer, length, type, m_head[0]));
}

void NetClient::send(const std::string& message, MessageType type)
{
	send(message.c_str(), message.size(), type);
}

bool NetClient::onFirstReceive(const char* buffer, int32_t length, MessageType type)
{
	return false;
}

void NetClient::receive(uv_tcp_t* sender, const char* buffer, int32_t length)
{
	if (m_destServer == nullptr)
	{
		return;
	}
	NetWorkHelper::receive(sender, buffer, length, m_receiveArea, &m_receiveQueue, &m_receiveSemaphore, this);
}

void NetClient::onReceive(const char* buffer, int32_t length, MessageType type)
{
	
}

void NetClient::onServerConnected()
{

}

void NetClient::onServerNotFind()
{

}

void NetClient::onServerDisconnected()
{

}

void NetClient::uvServerConnected(uv_tcp_t* server)
{
	m_destServer = server;
	onServerConnected();
}

void NetClient::uvServerNotFind()
{
	onServerNotFind();
}

void NetClient::uvServerNotFindClear()
{
	if (m_heartThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_heartThreadId);
		m_heartThreadId = 0;
	}

	if (m_receiveThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_receiveThreadId);
		m_receiveThreadId = 0;
	}
}

void NetClient::uvServerDisconnected(uv_tcp_t* server)
{
	m_destServer = nullptr;
	onServerDisconnected();
}

void NetClient::uvDisconnectedClear(uv_tcp_t* tcp)
{
	m_isConnected = false;

	if (m_heartThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_heartThreadId);
		m_heartThreadId = 0;
	}

	if (m_receiveThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_receiveThreadId);
		m_receiveThreadId = 0;
	}
}

void NetClient::loop()
{
	std::shared_ptr<RunLoopTask> spRunLoopTask(new RunLoopTask);
	spRunLoopTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_loopThreadId)->PostTask(spRunLoopTask);
}

void NetClient::asyncClose()
{
	if (m_heartThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_heartThreadId);
		m_heartThreadId = 0;
	}

	if (m_receiveThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_receiveThreadId);
		m_receiveThreadId = 0;
	}

	if (m_loopThreadId != 0)
	{
		CTaskThreadManager::Instance().GetThreadInterface(m_loopThreadId)->StopAllTask();
	}

	m_isSendHeart = false;
	m_isConnected = false;
}

void NetClient::heart(int32_t time)
{
	if (m_heartThreadId == 0)
	{
		m_heartThreadId = CTaskThreadManager::Instance().Init();
	}
	std::shared_ptr<HeartTask> spHeartTask(new HeartTask);
	spHeartTask->setParam(time, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_heartThreadId)->PostTask(spHeartTask);
}

void NetClient::onHeart()
{

}