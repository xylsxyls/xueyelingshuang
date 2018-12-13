#include "LogReceive.h"
#include "LogManager/LogManagerAPI.h"
#include "NetSender/NetSenderAPI.h"
#include "../../NetServerManager/NetServerManager/ProtocolId.h"
#include "ProtoMessage/ProtoMessageAPI.h"

LogReceive::LogReceive():
m_netClientManagerPid(0)
{

}

void LogReceive::receive(char* buffer, int32_t length, int32_t sendPid, int32_t protocolId)
{
	printf("log receive\n");
	if (protocolId == ProtocolId::INIT)
	{
		printf("INIT, buffer = %s, length = %d\n", buffer, length);
		m_netClientManagerPid = sendPid;
		return;
	}
	if (sendPid == m_netClientManagerPid)
	{
		//receive
		switch (protocolId)
		{
		case ProtocolId::INIT:
		{
			break;
		}
		case ProtocolId::PROTO_MESSAGE:
		{
			ProtoMessage message;
			message.from(buffer);
			printf("receive from server buffer = %s", message.getMap().find("buffer")->second.toString().c_str());
			break;
		}
		default:
			break;
		}
		printf("receive, buffer = %s, length = %d\n", buffer, length);
		return;
	}
	printf("send to netclient\n");
	NetSender::instance().send(buffer, length, protocolId);
}