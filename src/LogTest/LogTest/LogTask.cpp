#include "LogTask.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "LogManager/LogManagerAPI.h"

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
	if ((int32_t)m_messageMap[LOG_IS_WRITE_LOG] == (int32_t)true)
	{
		if (m_isNet)
		{
			LogManager::instance().print((LogManager::LogLevel)(int32_t)m_messageMap[LOG_LEVEL],
				m_messageMap[LOG_FILE_NAME].toString(),
				m_messageMap[LOG_FUN_NAME].toString(),
				m_messageMap[LOG_PROCESS_NAME].toString(),
				m_messageMap[LOG_INT_DATE_TIME].toString(),
				(int32_t)(m_messageMap[LOG_THREAD_ID]),
				"NET %s %s",
				m_messageMap[LOG_LOGIN_NAME].toString().c_str(),
				m_messageMap[LOG_BUFFER].toString().c_str());
			return;
		}
		LogManager::instance().print((LogManager::LogLevel)(int32_t)m_messageMap[LOG_LEVEL],
			m_messageMap[LOG_FILE_NAME].toString(),
			m_messageMap[LOG_FUN_NAME].toString(),
			m_processName,
			m_messageMap[LOG_INT_DATE_TIME].toString(),
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