#include "ClientLineManager.h"

ClientLineManager::ClientLineManager()
{

}

ClientLineManager& ClientLineManager::instance()
{
	static ClientLineManager s_clientLineManager;
	return s_clientLineManager;
}

void ClientLineManager::addClient(int32_t connectId, int32_t clientPid, const std::string& clientName)
{
	std::unique_lock<std::mutex> lock(m_clientMutex);
	m_clientMap[connectId][clientPid] = clientName;
}