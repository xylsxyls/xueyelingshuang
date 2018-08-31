#pragma once
#include "LibuvTcpMacro.h"
#include <stdint.h>
#include <vector>
#include <mutex>
#include <map>
#include "ReadWriteMutex/ReadWriteMutexAPI.h"

typedef struct uv_tcp_s uv_tcp_t;
typedef struct uv_loop_s uv_loop_t;
typedef struct uv_async_s uv_async_t;
class ReceiveCallback;
struct Package;

class LibuvTcpAPI LibuvTcp
{
public:
	LibuvTcp();

	~LibuvTcp();

public:
	void initClient(const char* ip, int32_t port, ReceiveCallback* callback);

	void initServer(int32_t port, ReceiveCallback* callback, int32_t backlog = 128);

	void clientLoop();

	void serverLoop();

	void send(char* text);

	char* getText(uv_tcp_t* dest, char* buffer, int32_t length);

	ReceiveCallback* callback();

public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::vector<uv_loop_t*> m_vecServerLoop;
	ReadWriteMutex m_clientPtrToLoopMutex;
	std::mutex m_mu;
	std::map<uv_tcp_t*, uv_loop_t*> m_clientPtrToLoopMap;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	uv_loop_t* m_clientLoop;
	int32_t m_workIndex;
	int32_t m_coreCount;


protected:
	ReceiveCallback* m_receiveCallback;
	bool m_isClient;
};