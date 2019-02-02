#include "LogTask.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "LogManager/LogManagerAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"

LogTask::LogTask():
m_isNet(false)
{

}

void LogTask::DoTask()
{
	m_message.clear();
	m_message.from(m_buffer);
	m_messageMap.clear();
	m_message.getMap(m_messageMap);
	if ((int32_t)m_messageMap[LOG_UNINIT] == (int32_t)true)
	{
		LogManager::instance().deleteFile();
		return;
	}
	if ((int32_t)m_messageMap[LOG_IS_WRITE_LOG] == (int32_t)true)
	{
		uint64_t time = m_messageMap[LOG_INT_DATE_TIME];
		uint64_t* timePtr = &time;
		std::string logTime = IntDateTime(*(int32_t*)timePtr, *((int32_t*)timePtr + 1)).toString();
		if (m_isNet)
		{
			LogManager::instance().print((LogManager::LogLevel)(int32_t)m_messageMap[LOG_LEVEL],
				m_messageMap[LOG_FILE_NAME].toString(),
				m_messageMap[LOG_FUN_NAME].toString(),
				m_messageMap[LOG_PROCESS_NAME].toString() + ".exe",
				logTime,
				(int32_t)(m_messageMap[LOG_THREAD_ID]),
				"NET %s %s",
				m_messageMap[LOG_LOGIN_NAME].toString().c_str(),
				m_messageMap[LOG_BUFFER].toString().c_str());
			return;
		}
		LogManager::instance().print((LogManager::LogLevel)(int32_t)m_messageMap[LOG_LEVEL],
			m_messageMap[LOG_FILE_NAME].toString(),
			m_messageMap[LOG_FUN_NAME].toString(),
			m_processName + ".exe",
			logTime,
			(int32_t)(m_messageMap[LOG_THREAD_ID]),
			"%s",
			m_messageMap[LOG_BUFFER].toString().c_str());
	}
}

void LogTask::setParam(bool isNet, const std::string& buffer, const std::string& processName)
{
	m_isNet = isNet;
	m_buffer = buffer;
	m_processName = processName;
}