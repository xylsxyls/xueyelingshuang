#pragma once
#include "LibuvTcp/LibuvTcpAPI.h"

class ServerReceive : public ReceiveCallback
{
public:
	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length);
	virtual void clientConnected(uv_tcp_t* server);
};