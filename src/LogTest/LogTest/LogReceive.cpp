#include "LogReceive.h"
#include "CSystem/CSystemAPI.h"
#include "ScreenTask.h"
#include "LogTask.h"
#include "NetTask.h"
#include "LogManager/LogManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"

extern bool g_exit;

LogReceive::LogReceive():
m_screenSemaphore(nullptr),
m_screenQueue(nullptr),
m_logSemaphore(nullptr),
m_logQueue(nullptr),
m_netSemaphore(nullptr),
m_netQueue(nullptr),
m_lastLogTime(nullptr)
{
	
}

void LogReceive::receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type)
{
	std::string strBuffer(buffer, length);

	switch (type)
	{
	//工程消息
	case LOGTEST_MESSAGE:
	{
		m_message.clear();
		m_message.from(strBuffer);
		m_messageMap.clear();
		m_message.getMap(m_messageMap);
		if ((int32_t)m_messageMap[LOG_CLOSE] == (int32_t)true)
		{
			g_exit = true;
			return;
		}

		if ((int32_t)m_messageMap[LOG_SET] == (int32_t)true)
		{
			LogManager::instance().set(((int32_t)m_messageMap[LOG_SET_DEAL_LOG]) == 1, false);
			return;
		}
	}
	break;
	//发送给网络端
	case LOGTEST_SEND_MESSAGE:
	{
		m_screenQueue->push(strBuffer);
		m_screenSemaphore->signal();

		m_logQueue->push(strBuffer);
		m_logSemaphore->signal();

		*m_lastLogTime = CSystem::GetTickCount();

		m_netQueue->push(strBuffer);
		m_netSemaphore->signal();
	}
	break;
	case LOGTEST_LOCAL_MESSAGE:
	{
		m_screenQueue->push(strBuffer);
		m_screenSemaphore->signal();

		m_logQueue->push(strBuffer);
		m_logSemaphore->signal();

		*m_lastLogTime = CSystem::GetTickCount();
	}
	break;
	case LOGTEST_ONLY_MESSAGE:
	{
		m_logQueue->push(strBuffer);
		m_logSemaphore->signal();

		*m_lastLogTime = CSystem::GetTickCount();
	}
	break;
	default:
		break;
	}
}

void LogReceive::setArea(Semaphore* screenSemaphore,
	LockFreeQueue<std::string>* screenQueue,
	Semaphore* logSemaphore,
	LockFreeQueue<std::string>* logQueue,
	Semaphore* netSemaphore,
	LockFreeQueue<std::string>* netQueue)
{
	m_screenSemaphore = screenSemaphore;
	m_screenQueue = screenQueue;
	m_logSemaphore = logSemaphore;
	m_logQueue = logQueue;
	m_netSemaphore = netSemaphore;
	m_netQueue = netQueue;
}

void LogReceive::setLastLogTime(std::atomic<int32_t>* lastLogTime)
{
	m_lastLogTime = lastLogTime;
}