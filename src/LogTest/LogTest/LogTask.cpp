#include "LogTask.h"
#include "LogManager/LogManagerAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include <map>

LogTask::LogTask():
m_logSemaphore(nullptr),
m_logQueue(nullptr),
m_exit(false)
{

}

void LogTask::DoTask()
{
	static int32_t s_newFileId = 0;
	static std::map<std::string, int32_t> s_logDirFileIdMap;

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
			s_logDirFileIdMap.clear();
			s_newFileId = 0;
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
			int32_t fileId = 0;
			std::string logName = m_message.logname();
			if (!logName.empty())
			{
				std::string logDir = buildLogDir(logName);
				std::map<std::string, int32_t>::iterator it = s_logDirFileIdMap.find(logDir);
				if (it == s_logDirFileIdMap.end())
				{
					LogManagerConfig config;
					config.m_fileId = ++s_newFileId;
					config.m_path = logDir;
					LogManager::instance().init(config);
					fileId = LogManager::instance().findFileId(logDir);
					if (fileId != -1)
					{
						s_logDirFileIdMap[logDir] = fileId;
					}
					else
					{
						fileId = 0;
					}
				}
				else
				{
					fileId = it->second;
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

std::string LogTask::buildLogDir(const std::string& logName) const
{
	std::string logDir = logName;
	while (logDir.size() > 1 && (logDir[logDir.size() - 1] == '/' || logDir[logDir.size() - 1] == '\\'))
	{
		logDir.erase(logDir.size() - 1);
	}
	if (logDir.size() > 4 && CStringManager::Right(logDir, 4) == ".log")
	{
		logDir.erase(logDir.size() - 4);
	}
	while (logDir.size() > 1 && (logDir[logDir.size() - 1] == '/' || logDir[logDir.size() - 1] == '\\'))
	{
		logDir.erase(logDir.size() - 1);
	}
	if (!logDir.empty() && !isAbsolutePath(logDir))
	{
		logDir = CSystem::GetCurrentExePath() + logDir;
	}
	if (!logDir.empty() && logDir[logDir.size() - 1] != '/' && logDir[logDir.size() - 1] != '\\')
	{
#ifdef _WIN32
		logDir.append("\\");
#else
		logDir.append("/");
#endif
	}
	return logDir;
}

bool LogTask::isAbsolutePath(const std::string& path) const
{
	if (path.empty())
	{
		return false;
	}

#ifdef _WIN32
	bool isDriveAbsolute = path.size() > 2 && path[1] == ':' && (path[2] == '\\' || path[2] == '/');
	bool isUncAbsolute = path.size() > 1 && ((path[0] == '\\' && path[1] == '\\') || (path[0] == '/' && path[1] == '/'));
	return isDriveAbsolute || isUncAbsolute;
#else
	return path[0] == '/';
#endif
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