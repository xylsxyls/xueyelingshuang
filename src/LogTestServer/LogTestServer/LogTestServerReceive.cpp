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
	ProtoMessage message;
	message.from(std::string(buffer, length));
	switch (protocolId)
	{
	case CorrespondParam::CLIENT_INIT:
	{
		printf("CLIENT_INIT, length = %d\n", length);
		HandleInitMessage(message);
		break;
	}
	case CorrespondParam::SERVER_INIT:
	{
		printf("SERVER_INIT\n");
		break;
	}
	case CorrespondParam::PROTO_MESSAGE:
	{
		printf("PROTO_MESSAGE, length = %d\n", length);
		HandleMessage(message);
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

void LogTestServerReceive::HandleInitMessage(ProtoMessage& message)
{
	auto messageMap = message.getMap();
	std::string loginName = messageMap[LOGIN_NAME].toString();
	int32_t clientId = messageMap[CLIENT_ID];
	NetLineManager::instance().addConnect(loginName, clientId);
	printf("logtestserver init,loginName = %s,clientId = %d, send to netserver\n", loginName.c_str(), clientId);
	NetSender::instance().send(message.toString().c_str(), message.toString().length(), CorrespondParam::CLIENT_INIT, true);
}

void LogTestServerReceive::HandleMessage(ProtoMessage& message)
{
	std::string loginName = NetLineManager::instance().findLoginName((message.getMap())[CLIENT_ID]);
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
			int32_t& clientId = vecId[idIndex];
			message[CLIENT_ID] = clientId;
			message[LOGIN_NAME] = loginName;
			printf("send to netserver,computerName = %s,clientId = %d\n", computerName.c_str(), clientId);
			NetSender::instance().send(message.toString().c_str(), message.toString().length(), CorrespondParam::PROTO_MESSAGE, true);
		}
	}
}