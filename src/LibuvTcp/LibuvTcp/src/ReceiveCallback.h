#pragma once
#include "LibuvTcpMacro.h"
#include <stdint.h>

class LibuvTcp;
typedef struct uv_tcp_s uv_tcp_t;

class LibuvTcpAPI ReceiveCallback
{
public:
	ReceiveCallback();

public:
	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length);

	virtual void clientConnected(uv_tcp_t* client);

	virtual void serverConnected(uv_tcp_t* server);

public:
	LibuvTcp* m_libuvTcp;
};