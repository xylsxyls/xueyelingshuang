#include "LogTestServerReceive.h"
#include "NetSender/NetSenderAPI.h"
#include "NetLineManager.h"
#include "Cini/CiniAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"

LogTestServerReceive::LogTestServerReceive()
{
	m_iniPath = CSystem::GetCurrentExePath() + "Config.ini";
}

void LogTestServerReceive::receive(char* buffer, int32_t length, int32_t sendPid, CorrespondParam::ProtocolId protocolId)
{
	switch (protocolId)
	{
	case CorrespondParam::CLIENT_INIT:
	{
		printf("CLIENT_INIT, length = %d\n", length);
		ProtoMessage message;
		message.from(std::string(buffer + sizeof(int32_t), length - sizeof(int32_t)));
		int32_t clientId = getClientId(buffer, length);
		std::map<std::string, Variant> predefineMap;
		message.getMap(predefineMap, PREDEFINE);
		std::string loginName = predefineMap[LOGIN_NAME].toString();
		NetLineManager::instance().addConnect(loginName, clientId);
		std::string strMessage = set4ClientId(clientId);
		strMessage.append(message.toString());
		printf("logtest client init,loginName = %s,clientId = %d, send to netserver\n", loginName.c_str(), clientId);
		NetSender::instance().send(strMessage.c_str(), strMessage.length(), CorrespondParam::CLIENT_INIT, true);
		return;
	}
	case CorrespondParam::SERVER_INIT:
	{
		printf("SERVER_INIT\n");
		return;
	}
	case CorrespondParam::PROTO_MESSAGE:
	{
		//printf("PROTO_MESSAGE, length = %d\n", length);
		ProtoMessage message;
		message.from(std::string(buffer + sizeof(int32_t), length - sizeof(int32_t)));
		HandleMessage(message, getClientId(buffer, length));
		break;
	}
	case CorrespondParam::JSON:
	{
		break;
	}
	case CorrespondParam::XML:
	{
		break;
	}
	default:
		break;
	}
}

void LogTestServerReceive::HandleMessage(ProtoMessage& message, int32_t clientId)
{
	std::string loginName = NetLineManager::instance().findLoginName(clientId);
	Cini ini(m_iniPath);
	std::string collectComputerName = ini.ReadIni("CollectComputerName");
	std::vector<std::string> vecComputerName = CStringManager::split(collectComputerName, ",");
	int32_t index = -1;
	while (index++ != vecComputerName.size() - 1)
	{
		auto& computerName = vecComputerName[index];
		std::vector<int32_t> vecId = NetLineManager::instance().findConnect(computerName);
		int32_t idIndex = -1;
		while (idIndex++ != vecId.size() - 1)
		{
			message[LOG_LOGIN_NAME] = loginName;
			int32_t& clientId = vecId[idIndex];
			std::string strMessage = set4ClientId(clientId);
			strMessage.append(message.toString());
			//printf("send to netserver,computerName = %s,clientId = %d\n", computerName.c_str(), clientId);
			NetSender::instance().send(strMessage.c_str(), strMessage.length(), CorrespondParam::PROTO_MESSAGE, true);
		}
	}
}

int32_t LogTestServerReceive::getClientId(const char* buffer, int32_t length)
{
	if (length < 4)
	{
		return 0;
	}
	return *(int32_t*)buffer;
}

std::string LogTestServerReceive::set4ClientId(int32_t clientId)
{
	std::string strClientId;
	strClientId.resize(sizeof(int32_t));
	*(int32_t*)(&strClientId[0]) = clientId;
	return strClientId;
}