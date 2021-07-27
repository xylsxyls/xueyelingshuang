#include "LibuvBase.h"
#include "uv.h"
#include "LockFreeQueue/LockFreeQueueAPI.h"

LibuvBase::LibuvBase():
m_libuvTcp(nullptr),
m_loop(nullptr),
m_asyncHandle(nullptr),
m_queue(nullptr),
m_asyncCloseHandle(nullptr),
m_isClient(false)
{
	m_loop = new uv_loop_t;
	m_asyncHandle = new uv_async_t;
	m_queue = new LockFreeQueue<char *>;
	m_asyncCloseHandle = new uv_async_t;
}