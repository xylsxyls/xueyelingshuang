#include "ServerReceiveCallback.h"
#include "ProtoMessage/ProtoMessageAPI.h"

void ServerReceiveCallback::NetClientMessage(int32_t connectId, const char* buffer, int32_t length)
{
	printf("ServerReceiveCallback NetClientMessage connectId = %d, buffer = %s\n", connectId, buffer);
}

void ServerReceiveCallback::NetServerMessage(const char* buffer, int32_t length)
{
	printf("ServerReceiveCallback NetServerMessage buffer = %s\n", buffer);
}

void ServerReceiveCallback::receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type)
{
	switch (type)
	{
	case CLIENT_INIT:
	{
		std::string strMessage(buffer, length);
		ProtoMessage message;
		message.from(strMessage);
		int32_t connectId = message.getMap()[CONNECT_ID];
		int32_t clientPid = message.getMap()[CLIENT_PID];
		std::string clientInitInfo = message.getMap()[CLIENT_INIT_INFO].toString();
		clientInit(connectId, clientPid, clientInitInfo.c_str(), clientInitInfo.size());
		return;
	}
	break;
	case SERVER_INIT_RESPONSE:
	{
		serverInitResponse(buffer, length);
		return;
	}
	break;
	case NET_SERVER_MESSAGE:
	{
		NetServerMessage(buffer, length);
	}
	break;
	case NET_CLIENT_MESSAGE:
	{
		int32_t connectId = *(int32_t*)buffer;
		NetClientMessage(connectId, buffer + 4, length - 4);
	}
	break;
	case MESSAGE:
	{
		int32_t connectId = *(int32_t*)buffer;
		int32_t clientPid = *((int32_t*)buffer + 1);
		ClientMessage(connectId, clientPid, buffer + 8, length - 8);
	}
	break;
	default:
		break;
	}
}