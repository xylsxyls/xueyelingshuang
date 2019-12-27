#include "ClientPackageManager.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"

ClientPackageManager::ClientPackageManager():
m_clientId(0)
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
	++m_clientId;
	std::map<int32_t, Variant> predefineMap;
	message.getMap(predefineMap, PREDEFINE);
	std::string serverName = predefineMap[SERVER_NAME].toString();
	int32_t pid = 0;
	std::vector<int32_t> vecPid = CSystem::processPid(CStringManager::AnsiToUnicode(serverName));
	if (!vecPid.empty())
	{
		pid = vecPid[0];
	}
	m_serverPackageMap[m_clientId] = ClientPackage(predefineMap[CLIENT_NAME].toString(),
		serverName,
		pid,
		predefineMap[LOGIN_NAME].toString(), sender);
	m_clientPtrMap[sender] = m_clientId;
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

std::string ClientPackageManager::get4ClientId(uv_tcp_t* clientPtr)
{
	int32_t clientId = getClientId(clientPtr);
	std::string strClientId;
	strClientId.resize(sizeof(int32_t));
	*(int32_t*)(&strClientId[0]) = clientId;
	return strClientId;
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