#include "LogTestServerReceive.h"
#include "NetSender/NetSenderAPI.h"
#include "NetLineManager.h"
#include "Cini/CiniAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "LogTask.h"

LogTestServerReceive::LogTestServerReceive()
{
	m_iniPath = CSystem::GetCurrentExePath() + "Config.ini";
	m_logThread = CTaskThreadManager::Instance().GetThreadInterface(CTaskThreadManager::Instance().Init());
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
		std::map<int32_t, Variant> predefineMap;
		message.getMap(predefineMap, PREDEFINE);
		std::string loginName = predefineMap[LOGIN_NAME].toString();
		NetLineManager::instance().addConnect(loginName, clientId);
		std::string strMessage = set4ClientId(clientId);
		std::string protoMsg;
		message.toString(protoMsg);
		strMessage.append(protoMsg);
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
		LogTask* logTask = new LogTask;
		logTask->setParam(std::string(buffer + sizeof(int32_t), length - sizeof(int32_t)), getClientId(buffer, length), m_iniPath);
		std::shared_ptr<LogTask> spLogTask;
		spLogTask.reset(logTask);
		m_logThread->PostTask(spLogTask);
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