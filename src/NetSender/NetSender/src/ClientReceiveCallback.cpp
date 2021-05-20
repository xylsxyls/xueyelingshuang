#include "ClientReceiveCallback.h"
#include "ProtoMessage/ProtoMessageAPI.h"

void ClientReceiveCallback::NetClientMessage(const char* buffer, int32_t length)
{
	printf("NetClientMessage, buffer = %s\n", buffer);
}

void ClientReceiveCallback::NetServerMessage(const char* buffer, int32_t length)
{
	printf("NetServerMessage, buffer = %s\n", buffer);
}

void ClientReceiveCallback::receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type)
{
	switch (type)
	{
	case CLIENT_INIT_RESPONSE:
	{
		std::string strMessage(buffer, length);
		ProtoMessage message;
		message.from(strMessage);
		int32_t serverId = message.getMap()[SERVER_ID];
		std::string clientInitResponseInfo = message.getMap()[CLIENT_INIT_RESPONSE_INFO].toString();
		clientInitResponse(serverId, clientInitResponseInfo.c_str(), clientInitResponseInfo.size());
		return;
	}
	break;
	case SERVER_MESSAGE:
	{
		int32_t serverId = *(int32_t*)buffer;
		ServerMessage(serverId, buffer + 8, length - 8);
	}
	break;
	case NET_SERVER_MESSAGE:
	{
		NetServerMessage(buffer + 4, length - 4);
	}
	break;
	case NET_CLIENT_MESSAGE:
	{
		NetClientMessage(buffer, length);
	}
	break;
	default:
		break;
	}
}