#include "UserConfigManager.h"
#include "CStringManager/CStringManagerAPI.h"

UserConfigManager& UserConfigManager::instance()
{
	static UserConfigManager s_userConfigManager;
	return s_userConfigManager;
}

void UserConfigManager::setUserId(uint64_t userId)
{
	m_tableName = CStringManager::Format("user_%I64u", userId);
	createTableIfNotExist(m_tableName);
}