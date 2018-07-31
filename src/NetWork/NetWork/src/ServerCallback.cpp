#include "ServerCallback.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "NetServer.h"

ServerCallback::ServerCallback() :
m_callback(nullptr)
{
	m_callback = new ReceiveCallback;
}

void ServerCallback::receive(uv_tcp_t* client, char* buffer, int32_t length)
{

}

void ServerCallback::clientConnected(uv_tcp_t* client)
{

}
