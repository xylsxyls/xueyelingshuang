#include "ClientLineManager.h"

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