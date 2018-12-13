#include "LogTestServerReceive.h"
#include "NetSender/NetSenderAPI.h"
#include "../../NetServerManager/NetServerManager/ProtocolId.h"

void LogTestServerReceive::receive(char* buffer, int32_t length, int32_t sendPid, int32_t protocolId)
{
	printf("LogTestServerReceive\n");
	switch (protocolId)
	{
	case ProtocolId::INIT:
	{
		printf("INIT, buffer = %s, length = %d\n", buffer, length);
		break;
	}
	case ProtocolId::PROTO_MESSAGE:
	{
		printf("PROTO_MESSAGE, buffer = %s, length = %d\n", buffer, length);
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
	printf("send to netserver\n");
	NetSender::instance().send(buffer, length, protocolId, true);
}