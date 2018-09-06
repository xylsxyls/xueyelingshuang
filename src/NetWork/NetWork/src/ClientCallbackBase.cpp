#include "ClientCallbackBase.h"
#include "ClientCallback.h"
#include "NetClient.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"

void ClientCallbackBase::serverConnected(uv_tcp_t* server)
{
	if (m_callback == nullptr || ((ClientCallback*)m_callback)->netClient() == nullptr)
	{
		return;
	}

	{
		WriteLock areaWriteLock(*m_areaReadWriteMutex);
		m_area[server];
	}
	
	((ClientCallback*)m_callback)->netClient()->setServer(server);
	((ClientCallback*)m_callback)->serverConnected(server);
}