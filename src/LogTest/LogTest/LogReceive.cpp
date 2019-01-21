#include "LogReceive.h"
#include "LogManager/LogManagerAPI.h"
#include "NetSender/NetSenderAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CSystem/CSystemAPI.h"

LogReceive::LogReceive():
m_netClientManagerPid(0)
{
	m_netClientManagerPid = CSystem::processPid("NetClientManager1.0.exe");
}

void LogReceive::receive(char* buffer, int32_t length, int32_t sendPid, CorrespondParam::ProtocolId protocolId)
{
	if (sendPid == m_netClientManagerPid)
	{
		//从网络端接收
		receiveFromNet(buffer, length, protocolId);
	}
	else
	{
		//发送给网络端
		sendToNet(buffer, length, sendPid, protocolId);
	}
}

void LogReceive::receiveFromNet(char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId)
{
	printf("receive from net\n");
	switch (protocolId)
	{
	case CorrespondParam::CLIENT_INIT:
	{
		printf("CLIENT_INIT, length = %d\n", length);
		break;
	}
	case CorrespondParam::PROTO_MESSAGE:
	{
		ProtoMessage message;
		message.from(std::string(buffer, length));
		auto messageMap = message.getMap();
		std::string str = messageMap[LOG_BUFFER];
		if ((int32_t)messageMap[LOG_IS_SEND_SCREEN] == (int32_t)true)
		{
			RCSend("NET %s %s", messageMap[LOGIN_NAME].toString().c_str(), str.c_str());
		}
		if ((int32_t)messageMap[LOG_IS_WRITE_LOG] == (int32_t)true)
		{
			LogManager::instance().print((LogManager::LogLevel)(int32_t)messageMap[LOG_LEVEL],
				messageMap[LOG_FILE_NAME].toString(),
				messageMap[LOG_FUN_NAME].toString(),
				messageMap[LOG_PROCESS_NAME].toString(),
				messageMap[LOG_INT_DATE_TIME].toString(),
				(int32_t)(messageMap[LOG_THREAD_ID]),
				"NET %s %s",
				messageMap[LOGIN_NAME].toString().c_str(),
				str.c_str());
		}
		break;
	}
	default:
		break;
	}
}

void LogReceive::sendToNet(char* buffer, int32_t length, int32_t sendPid, CorrespondParam::ProtocolId protocolId)
{
	std::string processName = addSendMap(sendPid);
	ProtoMessage message;
	message.from(std::string(buffer, length));
	message[LOG_PROCESS_NAME] = processName;
	auto messageMap = message.getMap();
	std::string str = messageMap[LOG_BUFFER];
	if ((int32_t)messageMap[LOG_IS_SEND_SCREEN] == (int32_t)true)
	{
		RCSend("%s", str.c_str());
	}
	if ((int32_t)messageMap[LOG_IS_WRITE_LOG] == (int32_t)true)
	{
		LogManager::instance().print((LogManager::LogLevel)(int32_t)messageMap[LOG_LEVEL],
			messageMap[LOG_FILE_NAME].toString(),
			messageMap[LOG_FUN_NAME].toString(),
			processName,
			messageMap[LOG_INT_DATE_TIME].toString(),
			(int32_t)(messageMap[LOG_THREAD_ID]),
			"%s",
			str.c_str());
	}
	if ((int32_t)messageMap[LOG_IS_SEND_NET] == (int32_t)true)
	{
		printf("send to netclient\n");
		message[LOG_PROCESS_NAME] = processName;
		NetSender::instance().send(message.toString().c_str(), message.toString().length(), protocolId);
	}
}

std::string LogReceive::addSendMap(int32_t sendPid)
{
	auto it = m_sendMap.find(sendPid);
	if (it != m_sendMap.end())
	{
		return it->second;
	}
	std::string processName = CSystem::processName(sendPid);
	m_sendMap[sendPid] = processName;
	return processName;
}