#include "ProcessReceive.h"
#include "NetWork/NetWorkAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"

ProcessReceive::ProcessReceive():
m_netClient(nullptr)
{
	m_computerName = CSystem::getComputerName();
}

void ProcessReceive::receive(char* buffer, int32_t length, int32_t sendPid, CorrespondParam::ProtocolId protocalId)
{
	std::string clientName = CSystem::processName(sendPid);
	ProtoMessage message;
	switch (protocalId)
	{
	case CorrespondParam::CLIENT_INIT:
	{
		printf("PROCESS_CLIENT_INIT, length = %d\n", length);
		message.from(std::string(buffer, length));
		addClientServerLoginName(message, clientName);
		break;
	}
	case CorrespondParam::PROTO_MESSAGE:
	{
		printf("PROTO_MESSAGE, length = %d\n", length);
		message.from(std::string(buffer, length));
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
	printf("send to net\n");
	m_netClient->send(message.toString().c_str(), message.toString().length(), protocalId);
}

void ProcessReceive::setNetClient(NetClient* netClient)
{
	m_netClient = netClient;
}

void ProcessReceive::addClientServerLoginName(ProtoMessage& message, const std::string& clientName)
{
	std::string serverName = clientName;
	CStringManager::Insert(serverName, clientName.length() - 7, "Server");
	message[CLIENT_NAME] = clientName;
	message[SERVER_NAME] = serverName;
	message[LOGIN_NAME] = m_computerName;
}