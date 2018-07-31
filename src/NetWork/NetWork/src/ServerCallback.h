#pragma once
#include "NetWorkMacro.h"
#include "LibuvTcp/LibuvTcpAPI.h"

class NetWorkAPI ServerCallback : public ReceiveCallback
{
public:
	ServerCallback();
public:
	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length);

	virtual void clientConnected(uv_tcp_t* client);

protected:
	ReceiveCallback* m_callback;
};