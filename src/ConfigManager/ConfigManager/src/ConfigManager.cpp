#include "ConfigManager.h"
#include "SQLite/SQLiteAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "UserConfigManager.h"
#include "CGetPath/CGetPathAPI.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"

ConfigManager::ConfigManager():
m_key(0),
m_section(0),
m_spConfig(nullptr),
m_spProcessMutex(nullptr),
m_tableName("g_config")
{
	m_spProcessMutex.reset(new ProcessReadWriteMutex("ConfigManagerProcessMutex"));
}

ConfigManager& ConfigManager::instance()
{
	static ConfigManager s_configManager;
	return s_configManager;
}

ConfigManager& ConfigManager::operator[](int32_t sectionOrKey)
{
	if (m_key != 0)
	{
		m_section = m_key;
	}
	m_key = sectionOrKey;
	return *this;
}

ConfigManager ConfigManager::operator=(int32_t value)
{
	addConfig(m_key, value, m_section);
	m_key = 0;
	m_section = 0;
	return *this;
}

ConfigManager ConfigManager::operator=(uint64_t value)
{
	addConfig(m_key, value, m_section);
	m_key = 0;
	m_section = 0;
	return *this;
}

ConfigManager ConfigManager::operator=(double value)
{
	addConfig(m_key, value, m_section);
	m_key = 0;
	m_section = 0;
	return *this;
}

ConfigManager ConfigManager::operator=(const std::string& value)
{
	addConfig(m_key, value, m_section);
	m_key = 0;
	m_section = 0;
	return *this;
}

std::string ConfigManager::type()
{
	std::string result = getConfigType(m_key, m_section);
	m_key = 0;
	m_section = 0;
	return result;
}

int32_t ConfigManager::toInt32()
{
	int32_t result;
	getConfig(m_key, result, m_section);
	m_key = 0;
	m_section = 0;
	return result;
}

uint64_t ConfigManager::toUint64()
{
	uint64_t result;
	getConfig(m_key, result, m_section);
	m_key = 0;
	m_section = 0;
	return result;
}

double ConfigManager::toDouble()
{
	double result;
	getConfig(m_key, result, m_section);
	m_key = 0;
	m_section = 0;
	return result;
}

std::string ConfigManager::toString()
{
	std::string result;
	getConfig(m_key, result, m_section);
	m_key = 0;
	m_section = 0;
	return result;
}

void ConfigManager::init(const std::string& configPath)
{
	m_databasePath = configPath;
}

void ConfigManager::initSQLite()
{
	if (m_databasePath == "")
	{
		m_databasePath = CGetPath::GetCurrentExePath() + CGetPath::GetCurrentExeName() + ".db";
	}
	m_spConfig = nullptr;
	m_spConfig.reset(new SQLite(m_databasePath));
	createTableIfNotExist("g_config");
}

void ConfigManager::setUserId(uint64_t userId)
{
	m_tableName = CStringManager::Format("user_%I64u", userId);
	createTableIfNotExist(m_tableName);
}

UserConfigManager* ConfigManager::getUserConfigManager()
{
	return &(UserConfigManager::instance());
}

ConfigManager& ConfigManager::getGlobalConfig()
{
	m_tableName = "g_config";
	return *this;
}

void ConfigManager::addConfig(int32_t key, const std::string& value, int32_t section)
{
	addConfigBase(key, value, TYPE_BLOB, section);
}

void ConfigManager::getConfig(int32_t key, std::string& value, int32_t section)
{
	value = getConfigBase(key, section);
}

void ConfigManager::deleteConfig(int32_t key)
{
	deleteConfig(0, key);
}

void ConfigManager::deleteConfig(int32_t section, int32_t key)
{
	initSQLite();
	if (m_spConfig == nullptr)
	{
		return;
	}
	WriteLock writeLock(*m_spProcessMutex);
	m_spConfig->open();
	m_spConfig->execute(m_spConfig->preparedCreator(SQLiteSqlString::deleteString(m_tableName, CStringManager::Format("key='%d_%d'", section, key))));
	m_spConfig->close();
}

void ConfigManager::transaction()
{
	if (m_spConfig == nullptr)
	{
		return;
	}
	m_spConfig->transaction();
}

void ConfigManager::commit()
{
	if (m_spConfig == nullptr)
	{
		return;
	}
	m_spConfig->commit();
}

void ConfigManager::addConfig(int32_t key, int32_t value, int32_t section)
{
	addConfigBase(key, CStringManager::Format("%d", value), TYPE_INT32, section);
}

void ConfigManager::getConfig(int32_t key, int32_t& value, int32_t section)
{
	value = atoi(getConfigBase(key, section).c_str());
}

void ConfigManager::addConfig(int32_t key, uint64_t value, int32_t section)
{
	addConfigBase(key, CStringManager::Format("%I64u", value), TYPE_UINT64, section);
}

void ConfigManager::getConfig(int32_t key, uint64_t& value, int32_t section)
{
	value = CStringManager::atoui64(getConfigBase(key, section).c_str());
}

void ConfigManager::addConfig(int32_t key, double value, int32_t section)
{
	addConfigBase(key, CStringManager::Format("%lf", value), TYPE_DOUBLE, section);
}

void ConfigManager::getConfig(int32_t key, double& value, int32_t section)
{
	value = atof(getConfigBase(key, section).c_str());
}

void ConfigManager::addConfigBase(int32_t key, const std::string& value, const std::string& type, int32_t section)
{
	initSQLite();
	if (hasConfigBase(key, section))
	{
		updateConfigBase(key, value, type, section);
		return;
	}
	insertConfigBase(key, value, type, section);
}

bool ConfigManager::hasConfigBase(int32_t key, int32_t section)
{
	if (m_spConfig == nullptr)
	{
		return false;
	}
	WriteLock writeLock(*m_spProcessMutex);
	m_spConfig->open();
	auto result = m_spConfig->execute(m_spConfig->preparedCreator(SQLiteSqlString::selectString(m_tableName, "key", CStringManager::Format("key='%d_%d'", section, key))));
	if (!result.next())
	{
		m_spConfig->close();
		return false;
	}
	m_spConfig->close();
	return true;
}

bool ConfigManager::updateConfigBase(int32_t key, const std::string& value, const std::string& type, int32_t section)
{
	if (m_spConfig == nullptr)
	{
		return false;
	}
	WriteLock writeLock(*m_spProcessMutex);
	m_spConfig->open();
	std::string updateString = SQLiteSqlString::updateString(m_tableName.c_str(), "data,type", CStringManager::Format("key='%d_%d'", section, key));
	auto prepare = m_spConfig->preparedCreator(updateString);
	prepare.setBlob(0, value);
	prepare.setString(1, type);
	bool res = m_spConfig->execute(prepare).empty();
	m_spConfig->close();
	return res;
}

bool ConfigManager::insertConfigBase(int32_t key, const std::string& value, const std::string& type, int32_t section)
{
	if (m_spConfig == nullptr)
	{
		return false;
	}
	WriteLock writeLock(*m_spProcessMutex);
	m_spConfig->open();
	std::string insertString = SQLiteSqlString::insertString(m_tableName.c_str(), "key,data,type");
	auto prepare = m_spConfig->preparedCreator(insertString);
	prepare.setString(0, CStringManager::Format("%d_%d", section, key));
	prepare.setBlob(1, value);
	prepare.setString(2, type);
	bool res = m_spConfig->execute(prepare).empty();
	m_spConfig->close();
	return res;
}

std::string ConfigManager::getConfigBase(int32_t key, int32_t section)
{
	initSQLite();
	if (m_spConfig == nullptr)
	{
		return "";
	}
	WriteLock writeLock(*m_spProcessMutex);
	std::string selectString = SQLiteSqlString::selectString(m_tableName.c_str(), "data", CStringManager::Format("key='%d_%d'", section, key));
	m_spConfig->open();
	auto sqlResult = m_spConfig->execute(m_spConfig->preparedCreator(selectString));
	if (!sqlResult.next())
	{
		m_spConfig->close();
		return "";
	}
	std::string result = sqlResult.getBlob(0);
	m_spConfig->close();
	return result;
}

std::string ConfigManager::getConfigType(int32_t key, int32_t section)
{
	if (m_spConfig == nullptr)
	{
		return "";
	}
	std::string selectString = SQLiteSqlString::selectString(m_tableName.c_str(), "type", CStringManager::Format("key='%d_%d'", section, key));
	WriteLock writeLock(*m_spProcessMutex);
	m_spConfig->open();
	auto sqlResult = m_spConfig->execute(m_spConfig->preparedCreator(selectString));
	if (!sqlResult.next())
	{
		m_spConfig->close();
		return "";
	}
	std::string result = sqlResult.getString(0);
	m_spConfig->close();
	return result;
}

void ConfigManager::createTableIfNotExist(const std::string& tableName)
{
	if (m_spConfig == nullptr)
	{
		return;
	}
	std::vector<std::string> vecField;
	vecField.push_back("key varchar(20) primary key");
	vecField.push_back(CStringManager::Format("data blob(%u)", SAVE_LENGTH));
	vecField.push_back("type varchar(20)");
	std::string sqlString = SQLiteSqlString::createTableIfNotExistString(tableName.c_str(), vecField);
	WriteLock writeLock(*m_spProcessMutex);
	m_spConfig->open();
	m_spConfig->execute(m_spConfig->preparedCreator(sqlString));
	m_spConfig->close();
}

//#include "D:\\SendToMessageTest.h"
//
//int main()
//{
//	INIT_CONFIG("D:\\aaa.db");
//	GLOBAL_CONFIG[1] = 3;
//	GLOBAL_CONFIG[1][2] = 5;
//	GLOBAL_CONFIG[1][2] = 5.6;
//	std::string str("1234567");
//	str[3] = 0;
//	GLOBAL_CONFIG[1][2] = str;
//	std::string dds = GLOBAL_CONFIG[1][2].toString();
//	GLOBAL_CONFIG[1][3] = 5.6;
//	DELETE_GLOBAL_CONFIG(1, 2);
//	SET_CONFIG_USERID(121321321);
//	USER_CONFIG[1][3] = 1.2;
//	USER_CONFIG[1] = 1.2;
//	SET_CONFIG_USERID(121321322);
//	USER_CONFIG[1][2] = 1.2;
//	double sdddsd = USER_CONFIG[2][3].toDouble();
//	DELETE_USER_CONFIG(2, 3);
//	USER_CONFIG[2][3] = 1.2;
//	SET_CONFIG_USERID(1025);
//	USER_CONFIG[1][3] = 1.2;
//	DELETE_USER_CONFIG(2, 3);
//	DELETE_GLOBAL_CONFIG(1);
//	DELETE_GLOBAL_CONFIG(1, 3);
//	getchar();
//	return 0;
//}