#include "ServerManagerReceive.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "ProcessWork/ProcessWorkAPI.h"

ServerManagerReceive::ServerManagerReceive()
{

}

void ServerManagerReceive::clientConnected(uv_tcp_t* client)
{
	printf("clientConnected = %d\n", client);
}

void ServerManagerReceive::receive(uv_tcp_t* sender, char* buffer, int32_t length, int32_t protocolId)
{
	ProtoMessage message;
	std::string serverName;
	switch (protocolId)
	{
	case ProcessWork::INIT:
	{
		printf("NET_INIT, sender = %d, buffer = %s, length = %d\n", sender, buffer, length);
		message.from(buffer);
		serverName = eraseServerName(message);
		addClientPtr(message, sender);
		//NetLineManager::instance().addConnect(serverName, sender);
		break;
	}
	case ProcessWork::PROTO_MESSAGE:
	{
		printf("NET_PROTO_MESSAGE, sender = %d, buffer = %s, length = %d\n", sender, buffer, length);
		message.from(buffer);
		serverName = eraseServerName(message);
		addClientPtr(message, sender);
		break;
	}
	case ProcessWork::JSON:
	{
		break;
	}
	case ProcessWork::XML:
	{
		break;
	}
	default:
		break;
	}
	printf("send to process, serverName = %s\n", serverName.c_str());
	ProcessWork::instance().send(message.toString().c_str(), message.toString().length() , serverName, protocolId);
}

std::string ServerManagerReceive::eraseServerName(ProtoMessage& message)
{
	std::string serverName = message.getMap().find("ServerProcessName")->second.toString();
	auto messageMap = message.getMap();
	auto itServerName = messageMap.find("ServerProcessName");
	if (itServerName != messageMap.end())
	{
		messageMap.erase(itServerName);
	}
	message.setKeyMap(messageMap);
	return serverName;
}

void ServerManagerReceive::addClientPtr(ProtoMessage& message, uv_tcp_t* sender)
{
	message["ClientPtr"] = (int32_t)sender;
}