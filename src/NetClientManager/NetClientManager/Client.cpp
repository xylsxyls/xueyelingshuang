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

//从网络端接收
void Client::onReceive(char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId)
{
	//解压
	std::string strMessage;
	Compress::zlibUnCompress(strMessage, std::string(buffer, length));
	//获取pid
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
	ProcessWork::instance().post(clientPid, strMessage.c_str(), strMessage.length(), protocolId);
}