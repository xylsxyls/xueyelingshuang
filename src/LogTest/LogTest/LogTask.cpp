#include "LogTask.h"
#include "LogManager/LogManagerAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include "CStringManager/CStringManagerAPI.h"

LogTask::LogTask():
m_logSemaphore(nullptr),
m_logQueue(nullptr),
m_exit(false)
{

}

void LogTask::DoTask()
{
	while (!m_exit)
	{
		m_logSemaphore->wait();
		if (m_exit)
		{
			return;
		}

		m_logQueue->pop(&m_buffer);

		if (m_buffer == "logUninit")
		{
			LogManager::instance().uninitAll();
			continue;
		}

		if (m_buffer.size() >= 3 && CStringManager::Right(m_buffer, 3) == "NET")
		{
			m_buffer.pop_back();
			m_buffer.pop_back();
			m_buffer.pop_back();
			if (m_message.ParseFromString(m_buffer))
			{
				uint64_t time = m_message.logintdatetime();
				uint64_t* timePtr = &time;
				std::string logTime = IntDateTime(*(int32_t*)timePtr, *((int32_t*)timePtr + 1)).toString();
				LogManager::instance().print(0,
					(LogManager::LogLevel)(int32_t)m_message.loglevel(),
					m_message.logfilename(),
					m_message.logfunname(),
					m_message.logprocessname(),
					logTime,
					(int32_t)(m_message.logthreadid()),
					"NET %s %s",
					m_message.logloginname().c_str(),
					m_message.logbuffer().c_str());
			}
			continue;
		}

		if (m_message.ParseFromString(m_buffer))
		{
			static int32_t newFileId = 0;
			int32_t fileId = 0;
			std::string logName = m_message.logname();
			if (!logName.empty())
			{
				logName.append(".log");
				fileId = LogManager::instance().findFileId(logName);
				if (fileId == -1)
				{
					LogManager::instance().init(++newFileId, logName);
					fileId = newFileId;
				}
			}
			uint64_t time = m_message.logintdatetime();
			uint64_t* timePtr = &time;
			std::string logTime = IntDateTime(*(int32_t*)timePtr, *((int32_t*)timePtr + 1)).toString();
			LogManager::instance().print(fileId,
				(LogManager::LogLevel)(int32_t)m_message.loglevel(),
				m_message.logfilename(),
				m_message.logfunname(),
				m_message.logprocessname(),
				logTime,
				(int32_t)(m_message.logthreadid()),
				"%s",
				m_message.logbuffer().c_str());
		}
	}
}

void LogTask::StopTask()
{
	m_exit = true;
	m_logSemaphore->signal();
}

void LogTask::setParam(Semaphore* logSemaphore, LockFreeQueue<std::string>* logQueue)
{
	m_logSemaphore = logSemaphore;
	m_logQueue = logQueue;
}