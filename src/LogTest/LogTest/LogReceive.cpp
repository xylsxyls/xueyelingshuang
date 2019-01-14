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
		printf("INIT, buffer = %s, length = %d\n", buffer, length);
		break;
	}
	case NetSender::PROTO_MESSAGE:
	{
		ProtoMessage message;
		message.from(buffer);
		std::string str = message.getMap().find("buffer")->second.toString().c_str();
		RCSend("NET = %s", str.c_str());
		LOGINFO("NET = %s", str.c_str());
		break;
	}
	default:
		break;
	}
}

void LogReceive::sendToNet(char* buffer, int32_t length, int32_t sendPid, int32_t protocolId)
{
	printf("send to netclient\n");
	NetSender::instance().send(buffer, length, protocolId);
}