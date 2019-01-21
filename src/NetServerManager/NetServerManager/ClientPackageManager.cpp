#include "ClientPackageManager.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "CSystem/CSystemAPI.h"

int32_t ClientPackageManager::s_clientId = 0;

ClientPackageManager::ClientPackageManager()
{

}

ClientPackageManager& ClientPackageManager::instance()
{
	static ClientPackageManager s_clientPackageManager;
	return s_clientPackageManager;
}

void ClientPackageManager::addClientPackage(ProtoMessage& message, uv_tcp_t* sender)
{
	std::unique_lock<std::mutex> mu(m_mutex);
	++s_clientId;
	auto messageMap = message.getMap();
	std::string serverName = messageMap[SERVER_NAME].toString();
	m_serverPackageMap[s_clientId] = ClientPackage(messageMap[CLIENT_NAME].toString(), serverName, CSystem::processPid(serverName), messageMap[LOGIN_NAME].toString(), sender);
	m_clientPtrMap[sender] = s_clientId;
}

uv_tcp_t* ClientPackageManager::getClientPtr(int32_t clientId)
{
	auto itClient = m_serverPackageMap.find(clientId);
	if (itClient == m_serverPackageMap.end())
	{
		return nullptr;
	}
	return itClient->second.m_clientPtr;
}

int32_t ClientPackageManager::getClientId(uv_tcp_t* clientPtr)
{
	auto itClient = m_clientPtrMap.find(clientPtr);
	if (itClient == m_clientPtrMap.end())
	{
		return 0;
	}
	return itClient->second;
}

int32_t ClientPackageManager::getServerPid(uv_tcp_t* clientPtr)
{
	auto it = m_clientPtrMap.find(clientPtr);
	if (it == m_clientPtrMap.end())
	{
		return 0;
	}
	auto& clientId = it->second;
	auto itClient = m_serverPackageMap.find(clientId);
	if (itClient == m_serverPackageMap.end())
	{
		return 0;
	}
	return itClient->second.m_serverPid;
}