#include "ClientManagerReceive.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "CSystem/CSystemAPI.h"
//#include "Compress/CompressAPI.h"

void ClientManagerReceive::serverConnected(uv_tcp_t* server)
{
	printf("serverConnected = %d\n", server);
}

void ClientManagerReceive::receive(uv_tcp_t* sender, char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId)
{
	int32_t clientPid = 0;
	std::string strMessage = /*Compress::zlibUnCompress(*/std::string(buffer, length)/*)*/;
	switch (protocolId)
	{
	case CorrespondParam::CLIENT_INIT:
	{
		printf("CLIENT_INIT, length = %d\n", length);
		ProtoMessage message;
		message.from(strMessage);
		std::map<std::string, Variant> predefineMap;
		message.getMap(predefineMap, PREDEFINE);
		clientPid = CSystem::processPid(predefineMap[CLIENT_NAME].toString());
		addClientIdMap(sender, clientPid);
		std::string strMessage = message.toString();
		ProcessWork::instance().send(strMessage.c_str(), strMessage.length(), clientPid, protocolId);
		return;
	}
	case CorrespondParam::PROTO_MESSAGE:
	{
		//printf("PROTO_MESSAGE, length = %d\n", length);
		clientPid = findClientId(sender);
		break;
	}
	case CorrespondParam::JSON:
	{
		clientPid = findClientId(sender);
		break;
	}
	case CorrespondParam::XML:
	{
		clientPid = findClientId(sender);
		break;
	}
	default:
		break;
	}
	//printf("send to process\n");
	ProcessWork::instance().send(strMessage.c_str(), strMessage.length(), clientPid, protocolId);
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