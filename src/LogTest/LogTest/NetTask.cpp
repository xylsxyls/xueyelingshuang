#include "NetTask.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "NetSender/NetSenderAPI.h"

void NetTask::DoTask()
{
	m_message.clear();
	m_message.from(m_buffer);
	m_messageMap.clear();
	m_message.getMap(m_messageMap);
	if ((int32_t)m_messageMap[LOG_IS_SEND_NET] == (int32_t)true)
	{
		m_message[LOG_PROCESS_NAME] = m_processName;
		NetSender::instance().post(m_message);
	}
}

void NetTask::setParam(const std::string& buffer, const std::string& processName, CorrespondParam::ProtocolId protocolId)
{
	m_buffer = buffer;
	m_processName = processName;
	m_protocolId = protocolId;
}