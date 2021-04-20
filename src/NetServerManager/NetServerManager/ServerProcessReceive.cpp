#include "ServerProcessReceive.h"
#include "Server.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "Compress/CompressAPI.h"
#include "NetLineManager.h"

ServerProcessReceive::ServerProcessReceive():
m_server(nullptr)
{

}

//从本地服务器接收
void ServerProcessReceive::receive(int32_t sendPid, char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId)
{
	ProtoMessage message;
	message.from(std::string(buffer, length));
	uv_tcp_t* client = NetLineManager::instance().findClient((int32_t)message.getMap()[CONNECT_ID]);
	switch (protocolId)
	{
	case CorrespondParam::CLIENT_INIT:
	{
		printf("CLIENT_INIT process, length = %d\n", length);
	}
	break;
	case CorrespondParam::SERVER_INIT:
	{
		printf("SERVER_INIT process, length = %d\n", length);
		ProcessWork::instance().send(sendPid, buffer, length, protocolId);
		return;
	}
	default:
		break;
	}
	std::string strMessage;
	Compress::zlibCompress(strMessage, std::string(buffer, length), 9);
	m_server->send(client, strMessage.c_str(), strMessage.length(), protocolId);
}

void ServerProcessReceive::setServer(Server* server)
{
	m_server = server;
}