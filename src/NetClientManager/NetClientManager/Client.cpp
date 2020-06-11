#include "Client.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CSystem/CSystemAPI.h"
#include "Compress/CompressAPI.h"
#include "CStringManager/CStringManagerAPI.h"

void Client::onServerConnected()
{
	printf("serverConnected\n");
}

void Client::onReceive(char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId)
{
	//Ω‚—π
	std::string strMessage;
	Compress::zlibUnCompress(strMessage, std::string(buffer, length));
	//ªÒ»°pid
	ProtoMessage message;
	message.from(strMessage);
	int32_t clientPid = message.getMap()[CLIENT_PID];
	switch (protocolId)
	{
	case CorrespondParam::CLIENT_INIT:
	{
		printf("CLIENT_INIT NetClientManager, length = %d\n", length);
	}
	break;
	default:
		break;
	}
	ProcessWork::instance().send(clientPid, strMessage.c_str(), strMessage.length(), protocolId);
}