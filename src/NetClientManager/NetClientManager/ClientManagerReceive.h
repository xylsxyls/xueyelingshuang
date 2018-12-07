#pragma once
#include "NetWork/NetWorkAPI.h"

class ClientManagerReceive : public ClientCallback
{
public:
	virtual void serverConnected(uv_tcp_t* server);

	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length);
};