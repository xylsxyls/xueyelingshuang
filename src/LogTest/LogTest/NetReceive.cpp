#include "NetReceive.h"
#include "ScreenTask.h"
#include "LogTask.h"
#include "CSystem/CSystemAPI.h"
#include "Semaphore/SemaphoreAPI.h"

NetReceive::NetReceive():
m_initResponseSemaphore(nullptr),
m_screenMutex(nullptr),
m_screenSemaphore(nullptr),
m_screenQueue(nullptr),
m_logMutex(nullptr),
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

	{
		WriteLock writeLock(*m_screenMutex);
		m_screenQueue->push(strBuffer);
	}
	m_screenSemaphore->signal();

	{
		WriteLock writeLock(*m_logMutex);
		m_logQueue->push(strBuffer);
	}
	m_logSemaphore->signal();

	*m_lastLogTime = CSystem::GetTickCount();
}

void NetReceive::setInitResponseSemaphore(Semaphore* initResponseSemaphore)
{
	m_initResponseSemaphore = initResponseSemaphore;
}

void NetReceive::setArea(ReadWriteMutex* screenMutex,
	Semaphore* screenSemaphore,
	LockFreeQueue<std::string>* screenQueue,
	ReadWriteMutex* logMutex,
	Semaphore* logSemaphore,
	LockFreeQueue<std::string>* logQueue)
{
	m_screenMutex = screenMutex;
	m_screenSemaphore = screenSemaphore;
	m_screenQueue = screenQueue;
	m_logMutex = logMutex;
	m_logSemaphore = logSemaphore;
	m_logQueue = logQueue;
}

void NetReceive::setLastLogTime(std::atomic<int32_t>* lastLogTime)
{
	m_lastLogTime = lastLogTime;
}