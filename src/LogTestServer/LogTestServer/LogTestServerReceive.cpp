#include "LogTestServerReceive.h"
#include "NetSender/NetSenderAPI.h"
#include "NetLineManager.h"
#include "Cini/CiniAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"

LogTestServerReceive::LogTestServerReceive()
{
	m_iniPath = CSystem::GetCurrentExePath() + "LogTestServerConfig.ini";
}

void LogTestServerReceive::receive(int32_t sendPid, char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId)
{
	std::string strBuffer(buffer, length);
	ProtoMessage message;
	message.from(strBuffer);
	int32_t clientPid = message.getMap()[CLIENT_PID];
	int32_t connectId = message.getMap()[CONNECT_ID];
	switch (protocolId)
	{
	case CorrespondParam::CLIENT_INIT:
	{
		printf("CLIENT_INIT LogTestServer, length = %d\n", length);
		std::string loginName = message.getMap()[LOGIN_NAME];

		NetLineManager::instance().addConnect(loginName, clientPid, connectId);

		NetSender::instance().init(message, CorrespondParam::ProtocolId::CLIENT_INIT, true);
		return;
	}
	case CorrespondParam::SERVER_INIT:
	{
		printf("SERVER_INIT LogTestServer\n");
		return;
	}
	case CorrespondParam::PROTO_MESSAGE:
	{
		std::string loginName = NetLineManager::instance().findLoginName(clientPid, connectId);
		Cini ini(m_iniPath);
		std::string collectComputerName = ini.readIni("CollectComputerName");
		std::vector<std::string> vecComputerName = CStringManager::split(collectComputerName, ",");
		int32_t index = -1;
		while (index++ != vecComputerName.size() - 1)
		{
			auto& computerName = vecComputerName[index];
			std::vector<std::pair<int32_t, int32_t>> vecClient = NetLineManager::instance().findConnect(computerName);
			//因为没有断线机制，只发送最后一个同登录名用户
			if (vecClient.empty())
			{
				continue;
			}
			message[LOG_LOGIN_NAME] = loginName;
			message[CLIENT_PID] = vecClient.back().first;
			message[CONNECT_ID] = vecClient.back().second;
			NetSender::instance().post(message, true);
		}
		break;
	}
	default:
		break;
	}
}