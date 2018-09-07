#include "ServerCallback.h"
#include "LibuvTcp/LibuvTcpAPI.h"

ServerCallback::ServerCallback() :
m_callback(nullptr),
m_netServer(nullptr)
{
	m_callback = new ReceiveCallback;
}

void ServerCallback::clientConnected(uv_tcp_t* client)
{

}

void ServerCallback::setNetServer(NetServer* netServer)
{
	m_netServer = netServer;
}

NetServer* ServerCallback::netServer()
{
	return m_netServer;
}
