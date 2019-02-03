#include "ServerManagerReceive.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "ClientPackageManager.h"
#include "CStringManager/CStringManagerAPI.h"
#include "Compress/CompressAPI.h"

ServerManagerReceive::ServerManagerReceive()
{

}

void ServerManagerReceive::clientConnected(uv_tcp_t* client)
{
	printf("clientConnected = %d\n", client);
}

void ServerManagerReceive::receive(uv_tcp_t* sender, char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId)
{
	std::string strBuffer;
	Compress::zlibUnCompress(strBuffer, std::string(buffer, length));
	std::string strMessage;
	switch (protocolId)
	{
	case CorrespondParam::CLIENT_INIT:
	{
		printf("NET_CLIENT_INIT, sender = %d, length = %d\n", sender, length);
		ProtoMessage message;
		message.from(strBuffer);
		ClientPackageManager::instance().addClientPackage(message, sender);
		strMessage = ClientPackageManager::instance().get4ClientId(sender);
		std::string protoMsg;
		message.toString(protoMsg);
		strMessage.append(protoMsg);
		ProcessWork::instance().send(strMessage.c_str(), strMessage.length(), ClientPackageManager::instance().getServerPid(sender), protocolId);
		return;
	}
	case CorrespondParam::PROTO_MESSAGE:
	{
		//printf("NET_PROTO_MESSAGE, length = %d\n", length);
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
	//printf("send to process, serverPid = %d\n", ClientPackageManager::instance().getServerPid(sender));
	strMessage = ClientPackageManager::instance().get4ClientId(sender);
	strMessage.append(strBuffer);
	ProcessWork::instance().send(strMessage.c_str(), strMessage.length(), ClientPackageManager::instance().getServerPid(sender), protocolId);
}