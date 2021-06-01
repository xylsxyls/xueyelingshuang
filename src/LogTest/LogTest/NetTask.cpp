#include "NetTask.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "NetSender/NetSenderAPI.h"

NetTask::NetTask():
m_isDeal(true)
{

}

void NetTask::DoTask()
{
	m_message.clear();
	m_message.from(m_buffer);
	m_messageMap.clear();
	m_message.getMap(m_messageMap);
	if ((int32_t)m_messageMap[LOG_SET] == (int32_t)true)
	{
		m_isDeal = ((int32_t)m_messageMap[LOG_SET_DEAL_LOG] == 1);
		return;
	}
	if (!m_isDeal)
	{
		return;
	}
	if ((int32_t)m_messageMap[LOG_IS_SEND_NET] == (int32_t)true)
	{
		m_message[LOG_PROCESS_NAME] = m_processName;
		m_message[LOG_LOGIN_NAME] = m_loginName;
		std::string strMessage = m_message.toString();
		NetSender::instance().postServer(PROJECT_LOGTEST, strMessage.c_str(), strMessage.size());
	}
}

void NetTask::setParam(const std::string& buffer, const std::string& processName, const std::string& loginName)
{
	m_buffer = buffer;
	m_processName = processName;
	m_loginName = loginName;
}