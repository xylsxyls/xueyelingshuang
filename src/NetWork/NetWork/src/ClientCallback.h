#pragma once
#include "NetWorkMacro.h"
#include <stdint.h>

class NetClient;
typedef struct uv_tcp_s uv_tcp_t;
class ReceiveCallback;

class NetWorkAPI ClientCallback
{
	friend class ClientCallbackBase;
public:
	ClientCallback();
public:
	virtual void receive(uv_tcp_t* server, char* buffer, int32_t length);

	virtual void serverConnected(uv_tcp_t* server);

	void setNetClient(NetClient* netClient);

protected:
	NetClient* m_netClient;

private:
	ReceiveCallback* m_callback;
};