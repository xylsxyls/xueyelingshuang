#pragma once
#include "NetWorkMacro.h"
#include <stdint.h>

typedef struct uv_tcp_s uv_tcp_t;
class ReceiveCallback;

class NetWorkAPI ClientCallback
{
public:
	ClientCallback();
public:
	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length);

	virtual void serverConnected(uv_tcp_t* server);

protected:
	ReceiveCallback* m_callback;
};