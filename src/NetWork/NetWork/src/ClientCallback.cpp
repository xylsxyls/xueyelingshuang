#include "ClientCallback.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "NetClient.h"

ClientCallback::ClientCallback():
m_callback(nullptr)
{
	m_callback = new ReceiveCallback;
}

void ClientCallback::receive(uv_tcp_t* client, char* buffer, int32_t length)
{

}

void ClientCallback::serverConnected(uv_tcp_t* server)
{

}

