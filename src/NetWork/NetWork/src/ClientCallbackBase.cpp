#include "ClientCallbackBase.h"
#include "ClientCallback.h"
#include "NetClient.h"

void ClientCallbackBase::serverConnected(uv_tcp_t* server)
{
	if (m_callback == nullptr || ((ClientCallback*)m_callback)->m_netClient == nullptr)
	{
		return;
	}
	((ClientCallback*)m_callback)->m_netClient->setServer(server);
	((ClientCallback*)m_callback)->serverConnected(server);
}