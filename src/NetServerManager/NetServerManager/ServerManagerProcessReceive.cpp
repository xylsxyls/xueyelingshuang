#include "ServerManagerProcessReceive.h"
#include "ProtocolId.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CSystem/CSystemAPI.h"
#include "NetWork/NetWorkAPI.h"
#include <mutex>
#include "CStringManager/CStringManagerAPI.h"

ServerManagerProcessReceive::ServerManagerProcessReceive():
m_netServer(nullptr),
m_connectedMap(nullptr),
m_mapMutex(nullptr)
{

}

void ServerManagerProcessReceive::receive(char* buffer, int32_t length, int32_t sendPid, int32_t protocalId)
{
	RCSend("process receive\n");
	ProtoMessage message;
	uv_tcp_t* dest = nullptr;
	switch (protocalId)
	{
	case ProtocolId::INIT:
	{
		RCSend("INIT, buffer = %s, length = %d\n", buffer, length);
	}
	case ProtocolId::PROTO_MESSAGE:
	{
		RCSend("PROTO_MESSAGE, buffer = %s, length = %d\n", buffer, length);
		message.from(buffer);
		std::string serverName = CSystem::processName(sendPid);
		m_mapMutex->lock();
		dest = (*m_connectedMap)[serverName];
		m_mapMutex->unlock();
		std::string clientName = serverName;
		CStringManager::Delete(clientName, clientName.length() - 13, 6);
		message["ClientProcessName"] = clientName;
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
	RCSend("send to net, dest = %d\n", dest);
	m_netServer->send(message.toString().c_str(), message.toString().length(), protocalId, dest);
}

void ServerManagerProcessReceive::setServer(NetServer* netServer)
{
	m_netServer = netServer;
}

void ServerManagerProcessReceive::setConnectedMap(std::map<std::string, uv_tcp_t*>* connectedMap)
{
	m_connectedMap = connectedMap;
}

void ServerManagerProcessReceive::setMutex(std::mutex* mapMutex)
{
	m_mapMutex = mapMutex;
}