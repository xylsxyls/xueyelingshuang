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

void LogTestServerReceive::clientInit(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length)
{
	printf("CLIENT_INIT, connectId = %d, clientPid = %d, buffer = %s\n", connectId, clientPid, buffer);
	std::string loginName = buffer;

	NetLineManager::instance().addConnect(loginName, connectId, clientPid);

	NetSender::instance().sendClientInitResponse(connectId, clientPid, "LogTestServer has received");
}

void LogTestServerReceive::serverInitResponse(const char* buffer, int32_t length)
{
	printf("SERVER_INIT LogTestServer, buffer = %s\n", buffer);
}

void LogTestServerReceive::ClientMessage(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length)
{
	std::string loginName = NetLineManager::instance().findLoginName(connectId, clientPid);
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
		int32_t connectId = vecClient.back().first;
		int32_t clientPid = vecClient.back().second;
		NetSender::instance().postClient(connectId, clientPid, buffer, length);
	}
}