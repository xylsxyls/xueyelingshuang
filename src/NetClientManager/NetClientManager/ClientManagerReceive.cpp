#include "ClientManagerReceive.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "CSystem/CSystemAPI.h"

void ClientManagerReceive::serverConnected(uv_tcp_t* server)
{
	printf("serverConnected = %d\n", server);
}

void ClientManagerReceive::receive(uv_tcp_t* sender, char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId)
{
	ProtoMessage message;
	int32_t clientPid = 0;
	switch (protocolId)
	{
	case CorrespondParam::CLIENT_INIT:
	{
		printf("CLIENT_INIT, length = %d\n", length);
		message.from(std::string(buffer, length));
		auto messageMap = message.getMap();
		clientPid = CSystem::processPid(messageMap[CLIENT_NAME].toString());
		addClientIdMap(sender, clientPid);
		break;
	}
	case CorrespondParam::PROTO_MESSAGE:
	{
		printf("PROTO_MESSAGE, length = %d\n", length);
		message.from(std::string(buffer, length));
		clientPid = findClientId(sender);
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
	printf("send to process\n");
	ProcessWork::instance().send(message.toString().c_str(), message.toString().length(), clientPid, protocolId);
}

std::string ClientManagerReceive::removeClientName(ProtoMessage& message)
{
	std::string clientName;
	auto messageMap = message.getMap();
	auto itClientName = messageMap.find(CLIENT_NAME);
	if (itClientName != messageMap.end())
	{
		clientName = itClientName->second;
		messageMap.erase(itClientName);
	}
	message.setKeyMap(messageMap);
	return clientName;
}

void ClientManagerReceive::addClientIdMap(uv_tcp_t* server, int32_t clientPid)
{
	m_clientIdMap[server] = clientPid;
}

int32_t ClientManagerReceive::findClientId(uv_tcp_t* server)
{
	auto it = m_clientIdMap.find(server);
	if (it == m_clientIdMap.end())
	{
		return 0;
	}
	return it->second;
}