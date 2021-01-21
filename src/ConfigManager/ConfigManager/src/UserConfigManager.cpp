#include "UserConfigManager.h"
#include "CStringManager/CStringManagerAPI.h"

UserConfigManager& UserConfigManager::instance()
{
	static UserConfigManager s_userConfigManager;
	return s_userConfigManager;
}

void UserConfigManager::setUserId(uint64_t userId)
{
#ifdef _MSC_VER
	m_tableName = CStringManager::Format("user_%I64u", userId);
#elif __unix__
	m_tableName = CStringManager::Format("user_%llu", userId);
#endif
	createTableIfNotExist(m_tableName);
}