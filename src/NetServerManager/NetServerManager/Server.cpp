#include "Server.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "Compress/CompressAPI.h"
#include "NetLineManager.h"

void Server::onClientConnected(uv_tcp_t* client)
{
	printf("clientConnected = %d\n", client);
}

void Server::onReceive(uv_tcp_t* client, char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId)
{
	//解压
	std::string strMessage;
	Compress::zlibUnCompress(strMessage, std::string(buffer, length));

	//获取客户端pid
	ProtoMessage message;
	message.from(strMessage);
	int32_t clientPid = message.getMap()[CLIENT_PID];
	std::string serverName;

	switch (protocolId)
	{
	case CorrespondParam::CLIENT_INIT:
	{
		printf("CLIENT_INIT NetServerManager, client = %p, length = %d\n", client, length);
		std::map<int32_t, Variant>& clientInfo = m_clientInfoMap[client][clientPid];
		clientInfo[CLIENT_NAME] = message.getMap()[CLIENT_NAME].toString();
		clientInfo[SERVER_NAME] = message.getMap()[SERVER_NAME].toString();
		clientInfo[LOGIN_NAME] = message.getMap()[LOGIN_NAME].toString();
		serverName = clientInfo[SERVER_NAME].toString();
		int32_t connectId = NetLineManager::instance().addConnect(client);
		message[CONNECT_ID] = connectId;
	}
	break;
	case CorrespondParam::PROTO_MESSAGE:
	{
		serverName = m_clientInfoMap[client][clientPid][SERVER_NAME].toString();
		message[CONNECT_ID] = NetLineManager::instance().findConnectId(client);
	}
	break;
	default:
		break;
	}
	message.toString(strMessage);
	ProcessWork::instance().send(serverName, strMessage.c_str(), strMessage.length(), protocolId);
}