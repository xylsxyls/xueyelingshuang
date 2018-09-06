#pragma once
#include "LibuvTcpMacro.h"
#include <stdint.h>
#include <vector>
#include <map>
#include <atomic>

typedef struct uv_tcp_s uv_tcp_t;
typedef struct uv_loop_s uv_loop_t;
typedef struct uv_async_s uv_async_t;
typedef void(*uv_async_cb)(uv_async_t* handle);
class ReceiveCallback;
class ReadWriteMutex;
template<class QElmType>
class LockFreeQueue;

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
	int32_t m_coreCount;
	uv_loop_t* m_serverLoop;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::vector<uv_loop_t*> m_vecServerLoop;
	std::map<uv_loop_t*, uv_async_t*> m_loopToAsyncHandleMap;
	std::map<uv_loop_t*, LockFreeQueue<char*>*> m_loopToQueueMap;
	
	ReadWriteMutex* m_clientPtrToLoopMutex;
	std::map<uv_tcp_t*, uv_loop_t*> m_clientPtrToLoopMap;
	std::atomic<int32_t> m_workIndex;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	
	uv_loop_t* m_clientLoop;
	uv_async_t* m_asyncHandle;
	LockFreeQueue<char*>* m_queue;

protected:
	ReceiveCallback* m_receiveCallback;
	bool m_isClient;
};