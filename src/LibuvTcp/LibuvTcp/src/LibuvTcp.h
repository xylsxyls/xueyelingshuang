#pragma once
#include <stdint.h>
#include <mutex>
#include "LibuvTcpMacro.h"

typedef struct uv_tcp_s uv_tcp_t;
typedef struct uv_loop_s uv_loop_t;
typedef struct uv_async_s uv_async_t;
typedef void(*uv_async_cb)(uv_async_t* handle);
template<class QElmType>
class LockFreeQueue;

class LibuvTcpAPI LibuvTcp
{
public:
	LibuvTcp();

	virtual ~LibuvTcp();

public:
	void initClient(const char* ip, int32_t port);

	void initServer(int32_t port, int32_t backlog = 128);

	void loop();

	void send(uv_tcp_t* dest, const char* buffer, int32_t length, int32_t protocolId);

	uv_loop_t* loopPtr();

	bool isClient();

	void close(uv_tcp_t* tcp);

	virtual void receive(uv_tcp_t* sender, char* buffer, int32_t length);

	virtual void clientConnected(uv_tcp_t* client);

	virtual void serverConnected(uv_tcp_t* server);

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::mutex m_queueMutex;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	LockFreeQueue<char*>* m_queue;
	uv_loop_t* m_loop;
	uv_async_t* m_asyncHandle;
	bool m_isClient;
	uint32_t m_loopThreadId;
};