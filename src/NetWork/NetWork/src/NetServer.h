#pragma once
#include "NetWorkMacro.h"
#include <stdint.h>
#include <string>
#include <map>
#include <vector>
#include <list>
#include "ReadWriteMutex/ReadWriteMutexAPI.h"

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
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<uv_tcp_t*, uint32_t> m_clientPtrToThreadIdMap;
	ReadWriteMutex m_clientPtrToThreadIdMutex;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	ServerCallbackBase* m_serverCallbackBase;
	
};