#include "ClientManagerReceive.h"
#include "../../NetServerManager/NetServerManager/ProtocolId.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "ProcessWork/ProcessWorkAPI.h"

void ClientManagerReceive::serverConnected(uv_tcp_t* server)
{
	printf("serverConnected = %d\n", server);
}

void ClientManagerReceive::receive(uv_tcp_t* sender, char* buffer, int32_t length, int32_t protocolId)
{
	printf("ClientManagerReceive\n");
	ProtoMessage message;
	std::string clientName;
	switch (protocolId)
	{
	case ProtocolId::INIT:
	{
		printf("INIT, buffer = %s, length = %d\n", buffer, length);
	}
	case ProtocolId::PROTO_MESSAGE:
	{
		printf("PROTO_MESSAGE, buffer = %s, length = %d\n", buffer, length);
		message.from(buffer);
		clientName = message.getMap().find("ClientProcessName")->second.toString();
		auto messageMap = message.getMap();
		auto itClientName = messageMap.find("ClientProcessName");
		if (itClientName != messageMap.end())
		{
			messageMap.erase(itClientName);
		}
		message.setKeyMap(messageMap);
		break;
	}
	case ProtocolId::JSON:
	{
		break;
	}
	case ProtocolId::XML:
	{
		break;
	}
	default:
		break;
	}
	printf("send to process\n");
	ProcessWork::instance().send(message.toString().c_str(), message.toString().length(), clientName, protocolId);
}