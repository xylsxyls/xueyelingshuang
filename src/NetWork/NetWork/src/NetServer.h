#pragma once
#include "NetWorkMacro.h"
#include <stdint.h>
#include <string>
#include <map>
#include <vector>
#include <list>

class LibuvTcp;
class ServerCallback;
class ServerCallbackBase;
typedef struct uv_tcp_s uv_tcp_t;

class NetWorkAPI NetServer
{
public:
	NetServer();

public:
	void listen(int32_t port, ServerCallback* callback);

	void send(char* buffer, int32_t length, uv_tcp_t* dest);

	

protected:
	LibuvTcp* m_libuvTcp;

public:
	ServerCallbackBase* m_serverCallbackBase;
	
};