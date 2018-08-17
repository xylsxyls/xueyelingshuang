#include "ClientCallback.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "NetClient.h"

ClientCallback::ClientCallback():
m_callback(nullptr),
m_netClient(nullptr)
{
	m_callback = new ReceiveCallback;
}

void ClientCallback::serverConnected(uv_tcp_t* server)
{

}

void ClientCallback::setNetClient(NetClient* netClient)
{
	m_netClient = netClient;
}