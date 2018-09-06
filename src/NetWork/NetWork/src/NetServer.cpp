#include "NetServer.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "ServerCallback.h"
#include "D:\\SendToMessageTest.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "WorkTask.h"
#include "SendTask.h"
#include "NetWorkThreadManager.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"
#include "ServerCallbackBase.h"

std::mutex g_netServerMu;

NetServer::NetServer() :
m_libuvTcp(nullptr),
m_serverCallbackBase(nullptr),
m_clientPtrToThreadIdMutex(nullptr)
{
	m_libuvTcp = new LibuvTcp;
	m_serverCallbackBase = new ServerCallbackBase;
	NetWorkThreadManager::instance().init(m_libuvTcp->coreCount());
	m_clientPtrToThreadIdMutex = new ReadWriteMutex;
}

void NetServer::listen(int32_t port, ServerCallback* callback)
{
	if (m_libuvTcp == nullptr || m_serverCallbackBase == nullptr || callback == nullptr)
	{
		return;
	}
	m_serverCallbackBase->setCallback(callback);
	callback->setNetServer(this);
	m_libuvTcp->initServer(port, m_serverCallbackBase);
	m_libuvTcp->serverLoop();
}

void NetServer::send(char* buffer, int32_t length, uv_tcp_t* dest)
{
	if (m_libuvTcp == nullptr || dest == nullptr)
	{
		return;
	}

	uint32_t threadId = 0;
	{
		ReadLock clientToThreadIdReadLock(*m_clientPtrToThreadIdMutex);
		auto itThreadId = m_clientPtrToThreadIdMap.find(dest);
		if (itThreadId == m_clientPtrToThreadIdMap.end())
		{
			return;
		}
		threadId = itThreadId->second;
	}
	
	std::shared_ptr<SendTask> spTask;
	SendTask* task = new SendTask;
	task->setLibuvTcp(m_libuvTcp);
	task->setParam(m_libuvTcp->getText(dest, buffer, length));
	spTask.reset(task);
	NetWorkThreadManager::instance().postSendTaskToThread(threadId, spTask);
}

ReadWriteMutex* NetServer::clientPtrToThreadIdMutex()
{
	return m_clientPtrToThreadIdMutex;
}

std::map<uv_tcp_t*, uint32_t>& NetServer::clientPtrToThreadIdMap()
{
	return m_clientPtrToThreadIdMap;
}