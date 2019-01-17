#include "NetLineManager.h"

NetLineManager::NetLineManager()
{

}

NetLineManager& NetLineManager::instance()
{
	static NetLineManager s_netLineManager;
	return s_netLineManager;
}

void NetLineManager::addConnect(const std::string& serverName, uv_tcp_t* connect)
{
	std::unique_lock<std::mutex> mu(m_mutex);
	m_connectedMap[serverName] = connect;
}

uv_tcp_t* NetLineManager::findConnect(const std::string& serverName)
{
	std::unique_lock<std::mutex> mu(m_mutex);
	auto it = m_connectedMap.find(serverName);
	if (it == m_connectedMap.end())
	{
		return nullptr;
	}
	return it->second;
}