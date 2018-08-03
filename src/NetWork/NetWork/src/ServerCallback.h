#pragma once
#include "NetWorkMacro.h"
#include <stdint.h>

class NetServer;
typedef struct uv_tcp_s uv_tcp_t;
class ReceiveCallback;

class NetWorkAPI ServerCallback
{
public:
	ServerCallback();

public:
	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length);

	virtual void clientConnected(uv_tcp_t* client);

	void setNetServer(NetServer* netServer);

	NetServer* netServer();

protected:
	NetServer* m_netServer;

private:
	ReceiveCallback* m_callback;
};