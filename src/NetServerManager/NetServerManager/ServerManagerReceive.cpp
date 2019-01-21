#include "ServerManagerReceive.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "ClientPackageManager.h"


ServerManagerReceive::ServerManagerReceive()
{

}

void ServerManagerReceive::clientConnected(uv_tcp_t* client)
{
	printf("clientConnected = %d\n", client);
}

void ServerManagerReceive::receive(uv_tcp_t* sender, char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId)
{
	ProtoMessage message;
	int32_t serverPid = 0;
	switch (protocolId)
	{
	case CorrespondParam::CLIENT_INIT:
	{
		printf("NET_CLIENT_INIT, sender = %d, length = %d\n", sender, buffer, length);
		message.from(std::string(buffer, length));
		ClientPackageManager::instance().addClientPackage(message, sender);
		addClientId(message, sender);
		break;
	}
	case CorrespondParam::PROTO_MESSAGE:
	{
		printf("NET_PROTO_MESSAGE, sender = %d, length = %d\n", sender, buffer, length);
		message.from(std::string(buffer, length));
		addClientId(message, sender);
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
	printf("send to process, serverPid = %d\n", ClientPackageManager::instance().getServerPid(sender));
	ProcessWork::instance().send(message.toString().c_str(), message.toString().length(), ClientPackageManager::instance().getServerPid(sender), protocolId);
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

void ServerManagerReceive::addClientId(ProtoMessage& message, uv_tcp_t* sender)
{
	message[CLIENT_ID] = ClientPackageManager::instance().getClientId(sender);
}