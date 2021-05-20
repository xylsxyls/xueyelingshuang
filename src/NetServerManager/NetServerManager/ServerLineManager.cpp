#include "ServerLineManager.h"

ServerLineManager::ServerLineManager()
{

}

ServerLineManager& ServerLineManager::instance()
{
	static ServerLineManager s_serverLineManager;
	return s_serverLineManager;
}

void ServerLineManager::addServerId(int32_t serverId, const std::string& serverName)
{
	std::unique_lock<std::mutex> lock(m_serverIdMutex);
	m_serverIdMap[serverId] = serverName;
}

void ServerLineManager::addServerPid(int32_t serverPid, const std::string& serverName)
{
	std::unique_lock<std::mutex> lock(m_serverPidMutex);
	m_serverPidMap[serverName] = serverPid;
}

int32_t ServerLineManager::findServerId(const std::string& serverName)
{
	std::unique_lock<std::mutex> lock(m_serverIdMutex);
	for (auto it = m_serverIdMap.begin(); it != m_serverIdMap.end(); ++it)
	{
		if (it->second == serverName)
		{
			return it->first;
		}
	}
	return -1;
}

int32_t ServerLineManager::findServerId(int32_t serverPid)
{
	return findServerId(findServerNameFromPid(serverPid));
}

std::string ServerLineManager::findServerNameFromPid(int32_t serverPid)
{
	std::unique_lock<std::mutex> lock(m_serverPidMutex);
	for (auto it = m_serverPidMap.begin(); it != m_serverPidMap.end(); ++it)
	{
		if (it->second == serverPid)
		{
			return it->first;
		}
	}
	return "";
}

std::string ServerLineManager::findServerNameFromId(int32_t serverId)
{
	std::unique_lock<std::mutex> lock(m_serverIdMutex);
	auto it = m_serverIdMap.find(serverId);
	if (it == m_serverIdMap.end())
	{
		return "";
	}
	return it->second;
}

int32_t ServerLineManager::findServerPid(const std::string& serverName)
{
	std::unique_lock<std::mutex> lock(m_serverPidMutex);
	auto it = m_serverPidMap.find(serverName);
	if (it == m_serverPidMap.end())
	{
		return 0;
	}
	return it->second;
}

int32_t ServerLineManager::findServerPid(int32_t serverId)
{
	return findServerPid(findServerNameFromId(serverId));
}