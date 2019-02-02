#include "ProcessReceive.h"
#include "NetWork/NetWorkAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
//#include "Compress/CompressAPI.h"

ProcessReceive::ProcessReceive():
m_netClient(nullptr)
{
	m_computerName = CSystem::getComputerName();
}

void ProcessReceive::receive(char* buffer, int32_t length, int32_t sendPid, CorrespondParam::ProtocolId protocalId)
{
	std::string strBuffer(buffer, length);
	switch (protocalId)
	{
	case CorrespondParam::CLIENT_INIT:
	{
		printf("PROCESS_CLIENT_INIT, length = %d\n", length);
		ProtoMessage message;
		message.from(strBuffer);
		addClientServerLoginName(message, CSystem::processName(sendPid));
		std::string strMessage = /*Compress::zlibCompress(*/message.toString()/*, 7)*/;
		m_netClient->send(strMessage.c_str(), strMessage.length(), protocalId);
		return;
	}
	case CorrespondParam::PROTO_MESSAGE:
	{
		//printf("PROTO_MESSAGE, length = %d\n", length);
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
	
	std::string strMessage = /*Compress::zlibCompress(*/strBuffer/*, 7)*/;
	m_netClient->send(strMessage.c_str(), strMessage.length(), protocalId);
}

void ProcessReceive::setNetClient(NetClient* netClient)
{
	m_netClient = netClient;
}

void ProcessReceive::addClientServerLoginName(ProtoMessage& message, const std::string& clientName)
{
	std::string serverName = clientName;
	CStringManager::Insert(serverName, clientName.length() - 7, "Server");
	std::map<std::string, Variant> predefineMap;
	predefineMap[CLIENT_NAME] = clientName;
	predefineMap[SERVER_NAME] = serverName;
	predefineMap[LOGIN_NAME] = m_computerName;
	message.setKeyMap(predefineMap, PREDEFINE);
}