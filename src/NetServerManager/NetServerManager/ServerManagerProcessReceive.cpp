#include "ServerManagerProcessReceive.h"
#include "CSystem/CSystemAPI.h"
#include "NetWork/NetWorkAPI.h"
#include <mutex>
#include "CStringManager/CStringManagerAPI.h"
#include "ClientPackageManager.h"

ServerManagerProcessReceive::ServerManagerProcessReceive():
m_netServer(nullptr)
{

}

void ServerManagerProcessReceive::receive(char* buffer, int32_t length, int32_t sendPid, CorrespondParam::ProtocolId protocolId)
{
	ProtoMessage message;
	std::string serverName = CSystem::processName(sendPid);
	uv_tcp_t* dest = nullptr;
	switch (protocolId)
	{
	case CorrespondParam::CLIENT_INIT:
	{
		printf("PROCESS_CLIENT_INIT, length = %d\n", length);
		message.from(std::string(buffer, length));
		dest = getClientPtr(message);
		break;
	}
	case CorrespondParam::SERVER_INIT:
	{
		printf("PROCESS_SERVER_INIT, length = %d\n", length);
		message.from(std::string(buffer, length));
		ProcessWork::instance().send(message.toString().c_str(), message.toString().length(), sendPid, protocolId);
		return;
	}
	case CorrespondParam::PROTO_MESSAGE:
	{
		printf("PROCESS_PROTO_MESSAGE, length = %d\n", length);
		message.from(std::string(buffer, length));
		dest = getClientPtr(message);
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
	printf("send to net, dest = %d\n", dest);
	m_netServer->send(message.toString().c_str(), message.toString().length(), protocolId, dest);
}

void ServerManagerProcessReceive::setServer(NetServer* netServer)
{
	m_netServer = netServer;
}

void ServerManagerProcessReceive::setClientName(ProtoMessage& message, const std::string& serverName)
{
	std::string clientName = serverName;
	CStringManager::Delete(clientName, clientName.length() - 13, 6);
	message["ClientProcessName"] = clientName;
}

uv_tcp_t* ServerManagerProcessReceive::getClientPtr(ProtoMessage& message)
{
	auto messageMap = message.getMap();
	uv_tcp_t* result = nullptr;
	auto itMessage = messageMap.find(CLIENT_ID);
	if (itMessage != messageMap.end())
	{
		auto& clientId = itMessage->second;
		result = ClientPackageManager::instance().getClientPtr(clientId);
	}
	return result;
}