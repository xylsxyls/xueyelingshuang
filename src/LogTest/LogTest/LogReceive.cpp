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

void LogReceive::receive(char* buffer, int32_t length, int32_t sendPid, int32_t protocolId)
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

void LogReceive::receiveFromNet(char* buffer, int32_t length, int32_t protocolId)
{
	printf("receive from net\n");
	switch (protocolId)
	{
	case NetSender::INIT:
	{
		printf("INIT, length = %d\n", buffer, length);
		break;
	}
	case NetSender::PROTO_MESSAGE:
	{
		ProtoMessage message;
		message.from(std::string(buffer, length));
		auto messageMap = message.getMap();
		std::string str = messageMap["buffer"];
		if ((int32_t)messageMap["isSendScreen"] == (int32_t)true)
		{
			RCSend("NET %s %s", messageMap["loginName"].toString().c_str(), str.c_str());
		}
		if ((int32_t)messageMap["isWriteLog"] == (int32_t)true)
		{
			LogManager::instance().print((LogManager::LogLevel)(int32_t)messageMap["logLevel"],
				messageMap["fileName"].toString(),
				messageMap["funName"].toString(),
				"NET %s %s",
				messageMap["loginName"].toString().c_str(),
				str.c_str());
		}
		break;
	}
	default:
		break;
	}
}

void LogReceive::sendToNet(char* buffer, int32_t length, int32_t sendPid, int32_t protocolId)
{
	ProtoMessage message;
	message.from(std::string(buffer, length));
	auto messageMap = message.getMap();
	std::string str = messageMap["buffer"];
	if ((int32_t)messageMap["isSendScreen"] == (int32_t)true)
	{
		RCSend("%s", str.c_str());
	}
	if ((int32_t)messageMap["isWriteLog"] == (int32_t)true)
	{
		LogManager::instance().print((LogManager::LogLevel)(int32_t)messageMap["logLevel"],
			messageMap["fileName"].toString(),
			messageMap["funName"].toString(),
			"%s %s",
			messageMap["loginName"].toString().c_str(),
			str.c_str());
	}
	if ((int32_t)messageMap["isSendNet"] == (int32_t)true)
	{
		printf("send to netclient\n");
		NetSender::instance().send(buffer, length, protocolId);
	}
}