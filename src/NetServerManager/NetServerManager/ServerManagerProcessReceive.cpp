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
	uv_tcp_t* dest = nullptr;
	switch (protocolId)
	{
	case CorrespondParam::CLIENT_INIT:
	{
		printf("PROCESS_CLIENT_INIT, length = %d\n", length);
		m_netServer->send(buffer + sizeof(int32_t), length - sizeof(int32_t), protocolId, getClientPtr(buffer, length));
		return;
	}
	case CorrespondParam::SERVER_INIT:
	{
		printf("PROCESS_SERVER_INIT, length = %d\n", length);
		ProcessWork::instance().send(buffer, length, sendPid, protocolId);
		return;
	}
	case CorrespondParam::PROTO_MESSAGE:
	{
		//printf("PROCESS_PROTO_MESSAGE, length = %d\n", length);
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
	//printf("send to net, dest = %d\n", dest);
	dest = getClientPtr(buffer, length);
	m_netServer->send(buffer + sizeof(int32_t), length - sizeof(int32_t), protocolId, dest);
}

void ServerManagerProcessReceive::setServer(NetServer* netServer)
{
	m_netServer = netServer;
}

uv_tcp_t* ServerManagerProcessReceive::getClientPtr(const char* buffer, int32_t length)
{
	if (length < sizeof(int32_t))
	{
		return nullptr;
	}
	return ClientPackageManager::instance().getClientPtr(*((int32_t*)buffer));
}