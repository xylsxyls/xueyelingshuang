#pragma once
#include "NetWork/NetWorkAPI.h"

class ServerManagerReceive : public ServerCallback
{
public:
	virtual void clientConnected(uv_tcp_t* client);

	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length);
};