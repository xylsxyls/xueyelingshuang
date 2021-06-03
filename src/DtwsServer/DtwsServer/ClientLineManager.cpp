#include "ClientLineManager.h"
#include <vector>

ClientLineManager::ClientLineManager()
{

}

ClientLineManager& ClientLineManager::instance()
{
	static ClientLineManager s_clientLineManager;
	return s_clientLineManager;
}

void ClientLineManager::addClient(const std::string& loginName, int32_t connectId, int32_t clientPid)
{
	std::unique_lock<std::mutex> lock(m_clientMutex);
	m_client[loginName] = std::pair<int32_t, int32_t>(connectId, clientPid);
}

std::vector<std::pair<int32_t, int32_t>> ClientLineManager::allClient()
{
	std::vector<std::pair<int32_t, int32_t>> result;
	std::unique_lock<std::mutex> lock(m_clientMutex);
	for (auto it = m_client.begin(); it != m_client.end(); ++it)
	{
		result.push_back(std::pair<int32_t, int32_t>(it->second.first, it->second.second));
	}
	return result;
}
