#pragma once
#include "NetWorkMacro.h"
#include <stdint.h>

class LibuvTcp;
class ServerCallback;
class ServerCallbackBase;
typedef struct uv_tcp_s uv_tcp_t;

class NetWorkAPI NetServer
{
public:
	NetServer();

public:
	void connect(const char* ip, int32_t port, ServerCallback* callback);

	void send(char* buffer, int32_t length, uv_tcp_t* dest = nullptr);

	void setServer(uv_tcp_t* server);

protected:
	LibuvTcp* m_libuvTcp;
	ServerCallbackBase* m_serverCallbackBase;
	uv_tcp_t* m_server;
};