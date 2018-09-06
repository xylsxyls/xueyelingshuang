#include "ClientCallback.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "NetClient.h"
#include "D:\\SendToMessageTest.h"

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