#include "NetClient.h"
#include "ClientCallback.h"
#include "SendTask.h"
#include "NetWorkThreadManager.h"
#include "ClientCallbackBase.h"

NetClient::NetClient() :
m_libuvTcp(nullptr),
m_clientCallbackBase(nullptr),
m_server(nullptr),
m_sendThreadId(0)
{
	m_libuvTcp = new LibuvTcp;
	m_clientCallbackBase = new ClientCallbackBase;
	NetWorkThreadManager::instance().init(m_libuvTcp->coreCount());
	m_sendThreadId = NetWorkThreadManager::instance().getWorkThreadId();
}

void NetClient::connect(const char* ip, int32_t port, ClientCallback* callback)
{
	if (m_libuvTcp == nullptr || m_clientCallbackBase == nullptr || callback == nullptr)
	{
		return;
	}
	m_clientCallbackBase->setCallback(callback);
	callback->setNetClient(this);
	m_libuvTcp->initClient(ip, port, m_clientCallbackBase);
	m_libuvTcp->clientLoop();
}

void NetClient::send(char* buffer, int32_t length, uv_tcp_t* dest)
{
	if (m_libuvTcp == nullptr)
	{
		return;
	}
	if (dest == nullptr)
	{
		dest = m_server;
	}
	if (m_server == nullptr)
	{
		return;
	}

	std::shared_ptr<SendTask> spTask;
	SendTask* task = new SendTask;
	task->setLibuvTcp(m_libuvTcp);
	task->setParam(m_libuvTcp->getText(dest, buffer, length));
	spTask.reset(task);

	NetWorkThreadManager::instance().postSendTaskToThread(m_sendThreadId, spTask);
}

void NetClient::setServer(uv_tcp_t* server)
{
	m_server = server;
}