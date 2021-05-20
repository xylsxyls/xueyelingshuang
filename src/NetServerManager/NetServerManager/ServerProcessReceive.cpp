#include "ServerProcessReceive.h"
#include "Server.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "Compress/CompressAPI.h"
#include "NetLineManager.h"
#include "ServerLineManager.h"
#include "CSystem/CSystemAPI.h"

ServerProcessReceive::ServerProcessReceive():
m_server(nullptr)
{

}

//从本地服务器接收
void ServerProcessReceive::receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type)
{
	switch (type)
	{
	case MessageType::CLIENT_INIT_RESPONSE:
	{
		printf("CLIENT_INIT_RESPONSE, length = %d\n", length);
		ProtoMessage message;
		message.from(std::string(buffer, length));
		int32_t connectId = message.getMap()[CONNECT_ID];
		uv_tcp_t* client = NetLineManager::instance().findClient(connectId);
		int32_t serverId = ServerLineManager::instance().findServerId(sendPid);
		message[SERVER_ID] = serverId;
		std::string strMessage;
		Compress::zlibCompress(strMessage, message.toString(), 9);
		m_server->send(client, strMessage.c_str(), strMessage.length(), type);
		return;
	}
	break;
	case MessageType::SERVER_INIT:
	{
		std::string serverName = CSystem::processName(sendPid);
		printf("SERVER_INIT processName = %s, buffer = %s\n", serverName.c_str(), buffer);
		ServerLineManager::instance().addServerPid(sendPid, serverName);
		ProcessWork::instance().send(sendPid, buffer, length, SERVER_INIT_RESPONSE);
		return;
	}
	case MessageType::SERVER_MESSAGE:
	{
		int32_t connectId = *(int32_t*)buffer;
		int32_t serverId = ServerLineManager::instance().findServerId(sendPid);
		*(int32_t*)buffer = serverId;
		uv_tcp_t* client = NetLineManager::instance().findClient(connectId);
		std::string strMessage;
		Compress::zlibCompress(strMessage, std::string(buffer, length), 9);
		m_server->send(client, strMessage.c_str(), strMessage.length(), type);
		return;
	}
	break;
	default:
		break;
	}
}

void ServerProcessReceive::setServer(Server* server)
{
	m_server = server;
}