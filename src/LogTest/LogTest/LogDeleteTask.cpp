#include "LogDeleteTask.h"
#include "CSystem/CSystemAPI.h"
#include "Semaphore/SemaphoreAPI.h"

LogDeleteTask::LogDeleteTask():
m_logSemaphore(nullptr),
m_logQueue(nullptr),
m_lastLogTime(nullptr),
m_exit(false)
{

}

void LogDeleteTask::DoTask()
{
	int32_t lastLogTime = 0;
	while (!m_exit)
	{
		if ((*m_lastLogTime != lastLogTime) && (CSystem::GetTickCount() - *m_lastLogTime > 5000) && ((*m_lastLogTime) != 0))
		{
			m_logQueue->push("logUninit");
			m_logSemaphore->signal();
			lastLogTime = *m_lastLogTime;
		}
		CSystem::Sleep(1);
	}
}

void LogDeleteTask::StopTask()
{
	m_exit = true;
}

void LogDeleteTask::setParam(Semaphore* logSemaphore,
	LockFreeQueue<std::string>* logQueue,
	std::atomic<int32_t>* lastLogTime)
{
	m_logSemaphore = logSemaphore;
	m_logQueue = logQueue;
	m_lastLogTime = lastLogTime;
}