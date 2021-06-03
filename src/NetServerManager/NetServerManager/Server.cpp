#include "Server.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "Compress/CompressAPI.h"
#include "NetLineManager.h"
#include "ServerLineManager.h"
#include "ClientLineManager.h"
#include "NetSender/NetSenderAPI.h"
#include <string.h>

void Server::onClientConnected(uv_tcp_t* client)
{
	printf("clientConnected = %p\n", client);
	NetLineManager::instance().addConnect(client);
}

//从网络端接收
void Server::onReceive(uv_tcp_t* client, const char* buffer, int32_t length, MessageType type)
{
	//解压
	std::string strMessage;
	Compress::zlibUnCompress(strMessage, std::string(buffer, length));

	//获取客户端pid
	int32_t serverPid = 0;

	switch (type)
	{
	case MessageType::CLIENT_INIT:
	{
		printf("CLIENT_INIT, client = %p, length = %d\n", client, length);
		//将连接ID和客户端进程号存入map，依据两个int值区分所有客户端
		//根据初始化过的serverName初始化消息中的SERVER_PID字段供客户端使用server代号
		ProtoMessage message;
		message.from(strMessage);
		int32_t clientPid = message.getMap()[CLIENT_PID];
		std::string serverName = message.getMap()[SERVER_NAME].toString();
		int32_t serverPid = ServerLineManager::instance().findServerPid(serverName);
		if (serverPid == 0)
		{
			const char* error = "server not init";
			std::string netServerMessage = NetSender::instance().netServerMessage(clientPid, error, strlen(error));
			std::string strMessage;
			Compress::zlibCompress(strMessage, netServerMessage, 9);
			send(client, strMessage.c_str(), strMessage.size(), NET_SERVER_MESSAGE);
			return;
		}

		std::string clientName = message.getMap()[CLIENT_NAME].toString();
		int32_t serverId = message.getMap()[SERVER_ID];

		ServerLineManager::instance().addServerId(serverId, serverName);
		int32_t connectId = NetLineManager::instance().findConnectId(client);
		message[CONNECT_ID] = connectId;
		ClientLineManager::instance().addClient(connectId, clientPid, clientName);

		std::string clientInitMessage = message.toString();
		ProcessWork::instance().post(serverPid, clientInitMessage.c_str(), clientInitMessage.length(), type);
		return;
	}
	break;
	case MessageType::MESSAGE:
	{
		int32_t serverId = *(int32_t*)(&strMessage[0]);
		serverPid = ServerLineManager::instance().findServerPid(serverId);
		int32_t connectId = NetLineManager::instance().findConnectId(client);
		*(int32_t*)(&strMessage[0]) = connectId;
	}
	break;
	default:
		break;
	}
	ProcessWork::instance().post(serverPid, strMessage.c_str(), strMessage.size(), type);
}