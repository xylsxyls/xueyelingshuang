#pragma once
#include <mutex>

class LibuvTcp;
typedef struct uv_tcp_s uv_tcp_t;
typedef struct uv_loop_s uv_loop_t;
typedef struct uv_async_s uv_async_t;
typedef struct uv_connect_s uv_connect_t;
template<class QElmType>
class LockFreeQueue;

struct LibuvBase
{
	LibuvTcp* m_libuvTcp;
	uv_loop_t* m_loop;
	uv_async_t* m_asyncHandle;
	std::mutex m_queueMutex;
	LockFreeQueue<char*>* m_queue;
	uv_async_t* m_asyncCloseHandle;
	bool m_isClient;

	LibuvBase();
};