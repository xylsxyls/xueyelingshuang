#include "NetLineManager.h"

NetLineManager::NetLineManager()
{

}

NetLineManager& NetLineManager::instance()
{
	static NetLineManager s_netLineManager;
	return s_netLineManager;
}

void NetLineManager::addConnect(const std::string& loginName, int32_t connectId, int32_t clientPid)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	m_loginNameMap[loginName].push_back(std::pair<int32_t, int32_t>(connectId, clientPid));
}

std::vector<std::pair<int32_t, int32_t>> NetLineManager::findConnect(const std::string& loginName)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	auto it = m_loginNameMap.find(loginName);
	if (it == m_loginNameMap.end())
	{
		return std::vector<std::pair<int32_t, int32_t>>();
	}
	return it->second;
}

std::string NetLineManager::findLoginName(int32_t connectId, int32_t clientPid)
{
	std::unique_lock<std::mutex> lock(m_mutex);
	for (auto it = m_loginNameMap.begin(); it != m_loginNameMap.end(); ++it)
	{
		if (it->second.empty())
		{
			continue;
		}
		if (it->second.back().first == connectId && it->second.back().second == clientPid)
		{
			return it->first;
		}
	}
	return "";
}