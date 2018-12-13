#include "ServerManagerReceive.h"
#include "ProtocolId.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "ProcessClient/ProcessClientAPI.h"
#include <mutex>

ServerManagerReceive::ServerManagerReceive():
m_connectedMap(nullptr),
m_mapMutex(nullptr)
{

}

void ServerManagerReceive::clientConnected(uv_tcp_t* client)
{
	RCSend("clientConnected = %d\n", client);
}

void ServerManagerReceive::receive(uv_tcp_t* sender, char* buffer, int32_t length, int32_t protocolId)
{
	RCSend("ServerManagerReceive\n");
	ProtoMessage message;
	std::string serverName;
	switch (protocolId)
	{
	case ProtocolId::INIT:
	{
		RCSend("INIT, sender = %d, buffer = %s, length = %d\n", sender, buffer, length);
		message.from(buffer);
		m_mapMutex->lock();
		(*m_connectedMap)[message.getMap().find("ServerProcessName")->second.toString()] = sender;
		m_mapMutex->unlock();
	}
	case ProtocolId::PROTO_MESSAGE:
	{
		RCSend("PROTO_MESSAGE, sender = %d, buffer = %s, length = %d\n", sender, buffer, length);
		message.from(buffer);
		serverName = message.getMap().find("ServerProcessName")->second.toString();
		auto messageMap = message.getMap();
		auto itServerName = messageMap.find("ServerProcessName");
		if (itServerName != messageMap.end())
		{
			messageMap.erase(itServerName);
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
	RCSend("send to process, serverName = %s\n", serverName.c_str());
	ProcessClient::instance().send(message.toString().c_str(), message.toString().length() , serverName, protocolId);
}

void ServerManagerReceive::setConnectedMap(std::map<std::string, uv_tcp_t*>* connectedMap)
{
	m_connectedMap = connectedMap;
}

void ServerManagerReceive::setMutex(std::mutex* mapMutex)
{
	m_mapMutex = mapMutex;
}