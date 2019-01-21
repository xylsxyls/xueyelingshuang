#include "NetLineManager.h"

NetLineManager::NetLineManager()
{

}

NetLineManager& NetLineManager::instance()
{
	static NetLineManager s_netLineManager;
	return s_netLineManager;
}

void NetLineManager::addConnect(const std::string& loginName, int32_t clientId)
{
	std::unique_lock<std::mutex> mu(m_mutex);
	m_connectedMap[loginName].push_back(clientId);
	m_loginNameMap[clientId] = loginName;
}

std::vector<int32_t> NetLineManager::findConnect(const std::string& loginName)
{
	std::unique_lock<std::mutex> mu(m_mutex);
	auto it = m_connectedMap.find(loginName);
	if (it == m_connectedMap.end())
	{
		return std::vector<int32_t>();
	}
	return it->second;
}

std::string NetLineManager::findLoginName(int32_t clientId)
{
	std::unique_lock<std::mutex> mu(m_mutex);
	auto it = m_loginNameMap.find(clientId);
	if (it == m_loginNameMap.end())
	{
		return "";
	}
	return it->second;
}