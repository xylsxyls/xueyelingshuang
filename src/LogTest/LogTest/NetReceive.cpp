#include "NetReceive.h"
#include "ScreenTask.h"
#include "LogTask.h"
#include "CSystem/CSystemAPI.h"
#include "Semaphore/SemaphoreAPI.h"

NetReceive::NetReceive():
m_initResponseSemaphore(nullptr),
m_screenSemaphore(nullptr),
m_screenQueue(nullptr),
m_logSemaphore(nullptr),
m_logQueue(nullptr),
m_lastLogTime(nullptr)
{
	
}

void NetReceive::clientInitResponse(int32_t serverId, const char* buffer, int32_t length)
{
	printf("CLIENT_INIT response, serverId = %d, buffer = %s, length = %d\n", serverId, buffer, length);
	m_initResponseSemaphore->signal();
}

void NetReceive::ServerMessage(int32_t serverId, const char* buffer, int32_t length)
{
	std::string strBuffer(buffer, length);
	strBuffer.append("NET");

	m_screenQueue->push(strBuffer);
	m_screenSemaphore->signal();

	m_logQueue->push(strBuffer);
	m_logSemaphore->signal();

	*m_lastLogTime = CSystem::GetTickCount();
}

void NetReceive::setInitResponseSemaphore(Semaphore* initResponseSemaphore)
{
	m_initResponseSemaphore = initResponseSemaphore;
}

void NetReceive::setArea(Semaphore* screenSemaphore,
	LockFreeQueue<std::string>* screenQueue,
	Semaphore* logSemaphore,
	LockFreeQueue<std::string>* logQueue)
{
	m_screenSemaphore = screenSemaphore;
	m_screenQueue = screenQueue;
	m_logSemaphore = logSemaphore;
	m_logQueue = logQueue;
}

void NetReceive::setLastLogTime(std::atomic<int32_t>* lastLogTime)
{
	m_lastLogTime = lastLogTime;
}