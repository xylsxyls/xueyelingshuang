#include "LogTask.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "NetLineManager.h"
#include "NetSender/NetSenderAPI.h"
#include "Cini/CiniAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"

LogTask::LogTask():
m_clientId(0)
{

}

void LogTask::DoTask()
{
	m_message.from(m_strMessage);
	std::string loginName = NetLineManager::instance().findLoginName(m_clientId);
	Cini ini(m_iniPath);
	std::string collectComputerName = ini.ReadIni("CollectComputerName");
	std::vector<std::string> vecComputerName = CStringManager::split(collectComputerName, ",");
	int32_t index = -1;
	while (index++ != vecComputerName.size() - 1)
	{
		auto& computerName = vecComputerName[index];
		std::vector<int32_t> vecId = NetLineManager::instance().findConnect(computerName);
		//因为没有断线机制，只发送最后一个同登录名用户
		if (vecId.empty())
		{
			continue;
		}
		m_message[LOG_LOGIN_NAME] = loginName;
		int32_t& clientId = vecId.back();
		std::string strMessage = set4ClientId(clientId);
		std::string protoMsg;
		m_message.toString(protoMsg);
		strMessage.append(protoMsg);
		//printf("send to netserver,computerName = %s,clientId = %d\n", computerName.c_str(), clientId);
		NetSender::instance().send(strMessage.c_str(), strMessage.length(), CorrespondParam::PROTO_MESSAGE, true);
	}
}

std::string LogTask::set4ClientId(int32_t clientId)
{
	std::string strClientId;
	strClientId.resize(sizeof(int32_t));
	*(int32_t*)(&strClientId[0]) = clientId;
	return strClientId;
}

void LogTask::setParam(const std::string& message, int32_t clientId, const std::string& iniPath)
{
	m_strMessage = message;
	m_clientId = clientId;
	m_iniPath = iniPath;
}