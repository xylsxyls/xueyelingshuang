#include "ClientCallback.h"

ClientCallback::ClientCallback():
m_netClient(nullptr)
{

}

void ClientCallback::serverConnected(uv_tcp_t* server)
{
	
}

void ClientCallback::setNetClient(NetClient* netClient)
{
	m_netClient = netClient;
}

NetClient* ClientCallback::netClient()
{
	return m_netClient;
}