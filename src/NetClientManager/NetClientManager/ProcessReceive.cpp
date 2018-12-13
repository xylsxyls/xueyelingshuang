#include "ProcessReceive.h"
#include "NetWork/NetWorkAPI.h"
#include "CSystem/CSystemAPI.h"
#include "../../NetServerManager/NetServerManager/ProtocolId.h"
#include "ProtoMessage/ProtoMessageAPI.h"

ProcessReceive::ProcessReceive():
m_netClient(nullptr)
{

}

void ProcessReceive::receive(char* buffer, int32_t length, int32_t sendPid, int32_t protocalId)
{
	printf("ProcessReceive\n");
	ProtoMessage message;
	switch (protocalId)
	{
	case ProtocolId::INIT:
	{
		printf("INIT, buffer = %s, length = %d\n", buffer, length);
		message.from(buffer);
		std::string clientName = CSystem::processName(sendPid);
		printf("clientName = %s\n", clientName.c_str());
		int32_t count = 4;
		while (count-- != 0)
		{
			clientName.pop_back();
		}
		message["ServerProcessName"] = clientName + "Server.exe";
		//printf("serverName = %s\n", message["ServerProcessName"].toString().c_str());
		break;
	}
	case ProtocolId::PROTO_MESSAGE:
	{
		printf("PROTO_MESSAGE, buffer = %s, length = %d\n", buffer, length);
		message.from(buffer);
		std::string clientName = CSystem::processName(sendPid);
		int32_t count = 4;
		while (count-- != 0)
		{
			clientName.pop_back();
		}
		message["ServerProcessName"] = clientName + "Server.exe";
		printf("process receive = %s", buffer);
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
	printf("send to net\n");
	m_netClient->send(message.toString().c_str(), message.toString().length(), protocalId);
}

void ProcessReceive::setNetClient(NetClient* netClient)
{
	m_netClient = netClient;
}