#include "NetLineManager.h"

NetLineManager::NetLineManager()
{

}

NetLineManager& NetLineManager::instance()
{
	static NetLineManager s_netLineManager;
	return s_netLineManager;
}

void NetLineManager::addConnect(const std::string& loginName, int32_t clientPid, int32_t connectId)
{
	m_connectedMap[std::pair<int32_t, int32_t>(clientPid, connectId)] = loginName;
	m_loginNameMap[loginName].push_back(std::pair<int32_t, int32_t>(clientPid, connectId));
}

std::vector<std::pair<int32_t, int32_t>> NetLineManager::findConnect(const std::string& loginName)
{
	auto it = m_loginNameMap.find(loginName);
	if (it == m_loginNameMap.end())
	{
		return std::vector<std::pair<int32_t, int32_t>>();
	}
	return it->second;
}

std::string NetLineManager::findLoginName(int32_t clientPid, int32_t connectId)
{
	auto it = m_connectedMap.find(std::pair<int32_t, int32_t>(clientPid, connectId));
	if (it == m_connectedMap.end())
	{
		return "";
	}
	return it->second;
}