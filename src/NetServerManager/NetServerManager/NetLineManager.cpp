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
	m_connectIdMap[++m_connectId] = client;
	m_connectClientMap[client] = m_connectId;
	return m_connectId;
}

uv_tcp_t* NetLineManager::findClient(int32_t connectId)
{
	auto itClient = m_connectIdMap.find(connectId);
	if (itClient == m_connectIdMap.end())
	{
		return nullptr;
	}
	return itClient->second;
}

int32_t NetLineManager::findConnectId(uv_tcp_t* client)
{
	auto itId = m_connectClientMap.find(client);
	if (itId == m_connectClientMap.end())
	{
		return 0;
	}
	return itId->second;
}