#include "Client.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CSystem/CSystemAPI.h"
#include "Compress/CompressAPI.h"
#include "CStringManager/CStringManagerAPI.h"

extern std::string g_ip;
extern int32_t g_port;

bool Client::onFirstReceive(const char* buffer, int32_t length, MessageType type)
{
	printf("%s\n", buffer);
	return true;
}

void Client::onFirstHead()
{
	printf("first head receive\n");
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
		printf("CLIENT_INIT_RESPONSE, clientPid = %d, length = %d\n", clientPid, length);
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

void Client::onServerConnected()
{
	printf("serverConnected\n");
	sendFirstMessage("NetManager Login");
}

void Client::onServerNotFind()
{
	connect(g_ip.c_str(), g_port, 2000);
}

void Client::onServerDisconnected()
{
	connect(g_ip.c_str(), g_port, 2000);
}