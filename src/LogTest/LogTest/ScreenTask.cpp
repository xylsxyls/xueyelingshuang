#include "ScreenTask.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "CStopWatch/CStopWatchAPI.h"

ScreenTask::ScreenTask():
m_isNet(false)
{

}

void ScreenTask::DoTask()
{
	m_message.clear();
	m_message.from(m_buffer);
	m_messageMap.clear();
	m_message.getMap(m_messageMap);
	if ((int32_t)m_messageMap[LOG_IS_SEND_SCREEN] == (int32_t)true)
	{
		if (m_isNet)
		{
			RCSend("NET %s %s", m_messageMap[LOG_LOGIN_NAME].toString().c_str(), m_messageMap[LOG_BUFFER].toString().c_str());
			return;
		}
		RCSend("%s", m_messageMap[LOG_BUFFER].toString().c_str());
	}
}

void ScreenTask::setParam(bool isNet, const std::string& buffer)
{
	m_isNet = isNet;
	m_buffer = buffer;
}