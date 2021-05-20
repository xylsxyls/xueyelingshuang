#include "Client.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CSystem/CSystemAPI.h"
#include "Compress/CompressAPI.h"
#include "CStringManager/CStringManagerAPI.h"

void Client::onServerConnected()
{
	printf("serverConnected\n");
}

//从网络端接收
void Client::onReceive(const char* buffer, int32_t length, MessageType type)
{
	//解压
	std::string strMessage;
	Compress::zlibUnCompress(strMessage, std::string(buffer, length));
	
	switch (type)
	{
	case MessageType::CLIENT_INIT_RESPONSE:
	{
		ProtoMessage message;
		message.from(strMessage);
		int32_t clientPid = message.getMap()[CLIENT_PID];
		printf("CLIENT_INIT_RESPONSE, length = %d\n", length);
		ProcessWork::instance().post(clientPid, strMessage.c_str(), strMessage.length(), type);
		return;
	}
	break;
	case MessageType::SERVER_MESSAGE:
	{
		int32_t clientPid = *((int32_t*)(&strMessage[0]) + 1);
		ProcessWork::instance().post(clientPid, strMessage.c_str(), strMessage.length(), type);
		return;
	}
	break;
	case MessageType::NET_SERVER_MESSAGE:
	{
		int32_t clientPid = *((int32_t*)(&strMessage[0]));
		if (clientPid == 0)
		{
			return;
		}
		ProcessWork::instance().post(clientPid, strMessage.c_str(), strMessage.length(), type);
		return;
	}
	break;
	default:
		break;
	}
}