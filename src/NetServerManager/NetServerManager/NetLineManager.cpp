#include "NetLineManager.h"

NetLineManager::NetLineManager():
m_connectId(0)
{

}

NetLineManager& NetLineManager::instance()
{
	static NetLineManager s_netLineManager;
	return s_netLineManager;
}

int32_t NetLineManager::addConnect(uv_tcp_t* client)
{
	std::unique_lock<std::mutex> lock(m_connectIdMutex);
	m_connectIdMap[++m_connectId] = client;
	return m_connectId;
}

uv_tcp_t* NetLineManager::findClient(int32_t connectId)
{
	std::unique_lock<std::mutex> lock(m_connectIdMutex);
	auto itClient = m_connectIdMap.find(connectId);
	if (itClient == m_connectIdMap.end())
	{
		return nullptr;
	}
	return itClient->second;
}

int32_t NetLineManager::findConnectId(uv_tcp_t* client)
{
	std::unique_lock<std::mutex> lock(m_connectIdMutex);
	for (auto itClient = m_connectIdMap.begin(); itClient != m_connectIdMap.end(); ++itClient)
	{
		if (itClient->second == client)
		{
			return itClient->first;
		}
	}
	return 0;
}