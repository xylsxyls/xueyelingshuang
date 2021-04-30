#include "Server.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "Compress/CompressAPI.h"
#include "NetLineManager.h"
#include "ServerLineManager.h"
#include "ClientLineManager.h"

void Server::onClientConnected(uv_tcp_t* client)
{
	printf("clientConnected = %p\n", client);
	NetLineManager::instance().addConnect(client);
}

//从网络端接收
void Server::onReceive(uv_tcp_t* client, const char* buffer, int32_t length, MessageType type)
{
	MessageType sendType = type;
	//解压
	std::string strMessage;
	Compress::zlibUnCompress(strMessage, std::string(buffer, length));

	//获取客户端pid
	int32_t serverPid = 0;

	switch (type)
	{
	case MessageType::CLIENT_INIT:
	{
		printf("NET_CLIENT_CLIENT_INIT, client = %p, length = %d\n", client, length);
		//将连接ID和客户端进程号存入map，依据两个int值区分所有客户端
		//根据初始化过的serverName初始化消息中的SERVER_PID字段供客户端使用server代号
		ProtoMessage message;
		message.from(strMessage);
		std::string serverName = message.getMap()[SERVER_NAME].toString();
		int32_t serverPid = ServerLineManager::instance().findServerPid(serverName);
		if (serverPid == 0)
		{
			const char* error = "server not init";
			send(client, error, strlen(error), NET_SERVER_MESSAGE);
			return;
		}

		std::string clientName = message.getMap()[CLIENT_NAME].toString();
		int32_t clientPid = message.getMap()[CLIENT_PID];
		int32_t serverId = message.getMap()[SERVER_ID];

		ServerLineManager::instance().addServerId(serverId, serverName);
		int32_t connectId = NetLineManager::instance().findConnectId(client);
		ClientLineManager::instance().addClient(connectId, clientPid, clientName);
		sendType = NET_SERVER_CLIENT_INIT;
		ProcessWork::instance().post(serverPid, strMessage.c_str(), strMessage.length(), sendType);
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
	ProcessWork::instance().post(serverPid, buffer, length, sendType);
}