#include "ServerManagerProcessReceive.h"
#include "CSystem/CSystemAPI.h"
#include "NetWork/NetWorkAPI.h"
#include <mutex>
#include "CStringManager/CStringManagerAPI.h"

ServerManagerProcessReceive::ServerManagerProcessReceive():
m_netServer(nullptr)
{

}

void ServerManagerProcessReceive::receive(char* buffer, int32_t length, int32_t sendPid, int32_t protocalId)
{
	ProtoMessage message;
	std::string serverName = CSystem::processName(sendPid);
	uv_tcp_t* dest = nullptr;
	switch (protocalId)
	{
	case ProcessWork::INIT:
	{
		printf("PROCESS_INIT, buffer = %s, length = %d\n", buffer, length);
		message.from(buffer);
		setClientName(message, serverName);
		dest = getClientPtr(message);
		break;
	}
	case ProcessWork::PROTO_MESSAGE:
	{
		printf("PROCESS_PROTO_MESSAGE, buffer = %s, length = %d\n", buffer, length);
		message.from(buffer);
		setClientName(message, serverName);
		dest = getClientPtr(message);
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
	printf("send to net, dest = %d\n", dest);
	m_netServer->send(message.toString().c_str(), message.toString().length(), protocalId, dest);
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
	auto itMessage = messageMap.find("ClientPtr");
	if (itMessage != messageMap.end())
	{
		result = (uv_tcp_t*)(int32_t)itMessage->second;
		messageMap.erase(itMessage);
		message.setKeyMap(messageMap);
	}
	return result;
}