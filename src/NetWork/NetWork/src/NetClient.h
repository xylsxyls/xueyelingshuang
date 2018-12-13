#pragma once
#include "NetWorkMacro.h"
#include <stdint.h>

class LibuvTcp;
class ClientCallback;
class ClientCallbackBase;
typedef struct uv_tcp_s uv_tcp_t;

class NetWorkAPI NetClient
{
public:
	NetClient();

public:
	void connect(const char* ip, int32_t port, ClientCallback* callback);
	void send(const char* buffer, int32_t length, int32_t protocolId = 1, uv_tcp_t* dest = nullptr);
	void setServer(uv_tcp_t* server);

protected:
	LibuvTcp* m_libuvTcp;
	ClientCallbackBase* m_clientCallbackBase;
	uv_tcp_t* m_server;
	uint32_t m_sendThreadId;
};