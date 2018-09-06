#include "ServerCallbackBase.h"
#include "ServerCallback.h"
#include "NetServer.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"
#include "NetWorkThreadManager.h"

void ServerCallbackBase::clientConnected(uv_tcp_t* client)
{
	if (m_callback == nullptr)
	{
		return;
	}

	ServerCallback* callback = (ServerCallback*)m_callback;
	{
		WriteLock areaWriteLock(*m_areaReadWriteMutex);
		m_area[client];
	}

	{
		WriteLock clientToThreadIdWriteLock(*(callback->netServer()->clientPtrToThreadIdMutex()));
		(callback->netServer()->clientPtrToThreadIdMap())[client] = NetWorkThreadManager::instance().getWorkThreadId();
	}
	callback->clientConnected(client);
}