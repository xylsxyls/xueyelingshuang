#include "CookStorageService.h"
#include "Config.h"
#include "CookStorageSyncTask.h"
#include "CookHelper.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <algorithm>
#include <cctype>
#include <sstream>

CookStorageService& CookStorageService::instance()
{
	static CookStorageService s_service;
	return s_service;
}

CookStorageService::CookStorageService() :
m_initialized(false),
m_enabled(false),
m_shuttingDown(false),
m_stateVersion(0),
m_nextSyncThreadIndex(0)
{

}

CookStorageService::~CookStorageService()
{
	shutdown();
}

bool CookStorageService::init()
{
	if (m_initialized.load())
	{
		return true;
	}
	m_initialized.store(true);
	if (!shouldUseRedisMysqlBackend())
	{
		m_initialized.store(false);
		LOGERROR("CookStorageService unsupported storage backend backend=%s, Redis+MySQL is required.",
		         g_config.m_storageBackend.c_str());
		return false;
	}

	m_enabled.store(true);
	m_shuttingDown.store(false);
	LOGINFO("CookStorageService init begin backend=%s mysqlHost=%s mysqlPort=%d mysqlDatabase=%s mysqlPoolSize=%d redisHost=%s redisPort=%d redisDb=%d redisPoolSize=%d syncThreads=%d",
	        g_config.m_storageBackend.c_str(),
	        g_config.m_mysqlHost.c_str(),
	        g_config.m_mysqlPort,
	        g_config.m_mysqlDatabase.c_str(),
	        g_config.m_mysqlPoolSize,
	        g_config.m_redisHost.c_str(),
	        g_config.m_redisPort,
	        g_config.m_redisDbIndex,
	        g_config.m_redisPoolSize,
	        g_config.m_storageSyncThreadCount);

	if (!initMysql())
	{
		m_enabled.store(false);
		m_initialized.store(false);
		LOGERROR("CookStorageService init mysql failed.");
		return false;
	}
	if (!initRedis())
	{
		m_enabled.store(false);
		m_initialized.store(false);
		m_mysqlPool.uninit();
		LOGERROR("CookStorageService init redis failed.");
		return false;
	}
	if (!initRedisSnapshotFromMysql())
	{
		m_enabled.store(false);
		m_initialized.store(false);
		m_redisPool.uninit();
		m_mysqlPool.uninit();
		LOGERROR("CookStorageService init redis snapshot failed.");
		return false;
	}
	if (!startSyncThreads())
	{
		m_enabled.store(false);
		m_initialized.store(false);
		m_redisPool.uninit();
		m_mysqlPool.uninit();
		LOGERROR("CookStorageService start sync threads failed.");
		return false;
	}

	LOGINFO("CookStorageService init success currentVersion=%lld",
	        static_cast<long long>(m_stateVersion.load()));
	return true;
}

void CookStorageService::shutdown()
{
	if (!m_initialized.load())
	{
		return;
	}
	if (!m_enabled.load())
	{
		m_initialized.store(false);
		return;
	}

	m_shuttingDown.store(true);
	LOGINFO("CookStorageService shutdown begin.");
	std::vector<uint32_t> threadIds;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		threadIds.swap(m_syncThreadIds);
		m_nextSyncThreadIndex = 0;
	}
	for (size_t i = 0; i < threadIds.size(); ++i)
	{
		CTaskThreadManager::Instance().WaitForEnd(threadIds[i]);
	}

	std::string redisStateText;
	int64_t redisVersion = 0;
	if (loadRedisStateSnapshot(redisStateText, redisVersion))
	{
		if (!writeMysqlStateSnapshot(redisStateText, redisVersion))
		{
			LOGERROR("CookStorageService final mysql sync failed version=%lld bytes=%d",
			         static_cast<long long>(redisVersion),
			         static_cast<int32_t>(redisStateText.size()));
		}
	}
	m_redisPool.uninit();
	m_mysqlPool.uninit();
	m_enabled.store(false);
	m_initialized.store(false);
	LOGINFO("CookStorageService shutdown complete.");
}

bool CookStorageService::isEnabled() const
{
	return m_enabled.load();
}

bool CookStorageService::loadAccountState(std::string& stateText)
{
	stateText.clear();
	if (!isEnabled())
	{
		return false;
	}

	int64_t version = 0;
	if (loadRedisStateSnapshot(stateText, version))
	{
		int64_t currentVersion = m_stateVersion.load();
		if (version > currentVersion)
		{
			m_stateVersion.store(version);
		}
		LOGINFO("CookStorageService load account state from redis version=%lld bytes=%d",
		        static_cast<long long>(version),
		        static_cast<int32_t>(stateText.size()));
		return true;
	}
	if (loadMysqlStateSnapshot(stateText, version))
	{
		int64_t currentVersion = m_stateVersion.load();
		if (version > currentVersion)
		{
			m_stateVersion.store(version);
		}
		writeRedisStateSnapshot(stateText, version);
		LOGINFO("CookStorageService load account state from mysql version=%lld bytes=%d",
		        static_cast<long long>(version),
		        static_cast<int32_t>(stateText.size()));
		return true;
	}
	return false;
}

bool CookStorageService::saveAccountStateAsync(const std::string& stateText)
{
	if (!isEnabled())
	{
		return false;
	}
	int64_t version = m_stateVersion.fetch_add(1) + 1;
	if (!writeRedisStateSnapshot(stateText, version))
	{
		LOGERROR("CookStorageService write redis failed, fallback mysql sync version=%lld bytes=%d",
		         static_cast<long long>(version),
		         static_cast<int32_t>(stateText.size()));
		return saveAccountStateSync(stateText);
	}

	std::shared_ptr<CookStorageSyncTask> task(new CookStorageSyncTask());
	task->setParam(stateText, version);
	uint32_t threadId = 0;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (!m_syncThreadIds.empty())
		{
			threadId = m_syncThreadIds[m_nextSyncThreadIndex++ % m_syncThreadIds.size()];
		}
	}
	std::shared_ptr<CTaskThread> thread = CTaskThreadManager::Instance().GetThreadInterface(threadId);
	if (thread == nullptr)
	{
		LOGERROR("CookStorageService sync thread missing, fallback mysql sync version=%lld", static_cast<long long>(version));
		return saveAccountStateSync(stateText);
	}
	thread->PostTask(task, g_config.m_storageSyncTaskPriority);
	LOGDEBUG("CookStorageService async state saved to redis and queued mysql sync version=%lld bytes=%d",
	         static_cast<long long>(version),
	         static_cast<int32_t>(stateText.size()));
	return true;
}

bool CookStorageService::saveAccountStateSync(const std::string& stateText)
{
	if (!isEnabled())
	{
		return false;
	}
	int64_t version = m_stateVersion.fetch_add(1) + 1;
	if (!writeRedisStateSnapshot(stateText, version))
	{
		LOGERROR("CookStorageService sync save redis write failed version=%lld", static_cast<long long>(version));
	}
	return writeMysqlStateSnapshot(stateText, version);
}

bool CookStorageService::syncStateSnapshotToMysql(const std::string& stateText, int64_t version)
{
	if (!isEnabled())
	{
		return false;
	}
	return writeMysqlStateSnapshot(stateText, version);
}

bool CookStorageService::shouldUseRedisMysqlBackend() const
{
	std::string backend = g_config.m_storageBackend;
	for (size_t i = 0; i < backend.size(); ++i)
	{
		backend[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(backend[i])));
	}
	return backend == "redis_mysql" || backend == "mysql_redis" || backend == "redis+mysql";
}

bool CookStorageService::initMysql()
{
	if (!isMysqlIdentifierValid(g_config.m_mysqlDatabase))
	{
		LOGERROR("CookStorageService invalid mysql database name database=%s", g_config.m_mysqlDatabase.c_str());
		return false;
	}
	if (!createMysqlDatabase())
	{
		return false;
	}
	MysqlCppConfig config = mysqlConfig(true);
	if (!m_mysqlPool.init(config, static_cast<size_t>(g_config.m_mysqlPoolSize)))
	{
		LOGERROR("CookStorageService mysql pool init failed error=%s", m_mysqlPool.lastError().c_str());
		return false;
	}
	return createMysqlTables();
}

bool CookStorageService::initRedis()
{
	HiRedisConfig config = redisConfig();
	if (!m_redisPool.init(config, static_cast<size_t>(g_config.m_redisPoolSize)))
	{
		LOGERROR("CookStorageService redis pool init failed error=%s", m_redisPool.lastError().c_str());
		return false;
	}
	std::shared_ptr<HiRedis> redis = acquireRedis();
	if (redis == nullptr || !redis->ping())
	{
		LOGERROR("CookStorageService redis ping failed error=%s", redis == nullptr ? "acquire failed" : redis->lastError().c_str());
		return false;
	}
	return true;
}

bool CookStorageService::startSyncThreads()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	for (int32_t i = 0; i < g_config.m_storageSyncThreadCount; ++i)
	{
		uint32_t threadId = CTaskThreadManager::Instance().Init();
		if (threadId == 0)
		{
			LOGERROR("CookStorageService create sync thread failed index=%d", i);
			return false;
		}
		m_syncThreadIds.push_back(threadId);
	}
	return true;
}

bool CookStorageService::createMysqlDatabase()
{
	MysqlCpp mysql;
	MysqlCppConfig config = mysqlConfig(false);
	if (!mysql.connect(config))
	{
		LOGERROR("CookStorageService mysql connect failed host=%s port=%d error=%s",
		         g_config.m_mysqlHost.c_str(),
		         g_config.m_mysqlPort,
		         mysql.lastError().c_str());
		return false;
	}
	std::string sql = "CREATE DATABASE IF NOT EXISTS " + g_config.m_mysqlDatabase + " CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci";
	std::shared_ptr<MysqlCppPrepareStatement> statement = mysql.prepare(sql);
	MysqlCppExecuteResult result = mysql.executeUpdate(statement, true);
	if (!result.m_success)
	{
		LOGERROR("CookStorageService create database failed database=%s error=%s",
		         g_config.m_mysqlDatabase.c_str(),
		         result.m_errorMessage.c_str());
		return false;
	}
	return true;
}

bool CookStorageService::createMysqlTables()
{
	std::shared_ptr<MysqlCpp> mysql = acquireMysql();
	if (mysql == nullptr)
	{
		return false;
	}
	std::string sql =
		"CREATE TABLE IF NOT EXISTS cook_state_snapshot ("
		"snapshotKey VARCHAR(64) PRIMARY KEY,"
		"snapshotValue LONGBLOB NOT NULL,"
		"updateVersion BIGINT NOT NULL DEFAULT 0,"
		"updateTime DATETIME NOT NULL"
		") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci";
	if (!executeMysqlUpdate(mysql, sql))
	{
		return false;
	}
	sql = "ALTER TABLE cook_state_snapshot MODIFY snapshotValue LONGBLOB NOT NULL";
	if (!executeMysqlUpdate(mysql, sql))
	{
		return false;
	}
	return true;
}

bool CookStorageService::initRedisSnapshotFromMysql()
{
	std::string redisStateText;
	std::string mysqlStateText;
	int64_t redisVersion = 0;
	int64_t mysqlVersion = 0;
	bool hasRedisState = loadRedisStateSnapshot(redisStateText, redisVersion);
	bool hasMysqlState = loadMysqlStateSnapshot(mysqlStateText, mysqlVersion);
	if (hasRedisState && (!hasMysqlState || redisVersion >= mysqlVersion))
	{
		m_stateVersion.store(redisVersion);
		if (!hasMysqlState || redisVersion > mysqlVersion)
		{
			writeMysqlStateSnapshot(redisStateText, redisVersion);
		}
		LOGINFO("CookStorageService keep redis snapshot version=%lld mysqlVersion=%lld",
		        static_cast<long long>(redisVersion),
		        static_cast<long long>(mysqlVersion));
		return true;
	}
	if (hasMysqlState)
	{
		m_stateVersion.store(mysqlVersion);
		writeRedisStateSnapshot(mysqlStateText, mysqlVersion);
		LOGINFO("CookStorageService initialize redis snapshot from mysql version=%lld bytes=%d",
		        static_cast<long long>(mysqlVersion),
		        static_cast<int32_t>(mysqlStateText.size()));
		return true;
	}
	m_stateVersion.store(0);
	LOGINFO("CookStorageService no existing mysql or redis state snapshot.");
	return true;
}

bool CookStorageService::loadMysqlStateSnapshot(std::string& stateText, int64_t& version)
{
	stateText.clear();
	version = 0;
	std::shared_ptr<MysqlCpp> mysql = acquireMysql();
	if (mysql == nullptr)
	{
		return false;
	}
	std::shared_ptr<MysqlCppPrepareStatement> statement = mysql->prepare(
		"SELECT snapshotValue, updateVersion FROM cook_state_snapshot WHERE snapshotKey=?");
	if (statement == nullptr || !statement->isValid())
	{
		LOGERROR("CookStorageService prepare mysql load snapshot failed error=%s", mysql->lastError().c_str());
		return false;
	}
	statement->setString(0, mysqlSnapshotKey());
	std::shared_ptr<MysqlCppResultSet> resultSet = mysql->query(statement, true);
	if (resultSet == nullptr || !resultSet->isValid())
	{
		return false;
	}
	if (!resultSet->next())
	{
		return false;
	}
	std::istream* blobStream = resultSet->getBlob("snapshotValue");
	if (blobStream == nullptr)
	{
		LOGERROR("CookStorageService mysql snapshot blob is null.");
		return false;
	}
	std::ostringstream oss;
	oss << blobStream->rdbuf();
	stateText = oss.str();
	version = resultSet->getInt64("updateVersion");
	return true;
}

bool CookStorageService::loadRedisStateSnapshot(std::string& stateText, int64_t& version)
{
	stateText.clear();
	version = 0;
	std::shared_ptr<HiRedis> redis = acquireRedis();
	if (redis == nullptr)
	{
		return false;
	}
	if (!redis->keyExist(redisSnapshotKey()))
	{
		return false;
	}
	std::shared_ptr<HiRedisResultSet> stateResult = redis->get(redisSnapshotKey());
	if (stateResult == nullptr || !stateResult->isValid() || stateResult->isNull())
	{
		return false;
	}
	std::shared_ptr<HiRedisResultSet> versionResult = redis->get(redisSnapshotVersionKey());
	stateText = stateResult->toString();
	if (versionResult != nullptr && versionResult->isValid() && !versionResult->isNull())
	{
		version = versionResult->toInt64();
	}
	return true;
}

bool CookStorageService::writeRedisStateSnapshot(const std::string& stateText, int64_t version)
{
	std::shared_ptr<HiRedis> redis = acquireRedis();
	if (redis == nullptr)
	{
		return false;
	}
	std::vector<HiRedisCommand> commands;
	commands.push_back(HiRedisCommand(std::vector<std::string>{"SET", redisSnapshotKey(), stateText}));
	commands.push_back(HiRedisCommand(std::vector<std::string>{"SET", redisSnapshotVersionKey(), CStringManager::toStringInt64(version)}));
	std::vector<std::shared_ptr<HiRedisResultSet>> results = redis->pipeline(commands);
	if (results.size() != commands.size())
	{
		LOGERROR("CookStorageService redis pipeline result size invalid size=%d expected=%d",
		         static_cast<int32_t>(results.size()),
		         static_cast<int32_t>(commands.size()));
		return false;
	}
	for (size_t i = 0; i < results.size(); ++i)
	{
		if (results[i] == nullptr || !results[i]->toStatus())
		{
			LOGERROR("CookStorageService redis set snapshot failed index=%d error=%s",
			         static_cast<int32_t>(i),
			         results[i] == nullptr ? "null result" : results[i]->error().c_str());
			return false;
		}
	}
	return true;
}

bool CookStorageService::writeMysqlStateSnapshot(const std::string& stateText, int64_t version)
{
	std::shared_ptr<MysqlCpp> mysql = acquireMysql();
	if (mysql == nullptr)
	{
		return false;
	}
	std::shared_ptr<MysqlCppPrepareStatement> statement = mysql->prepare(
		"INSERT INTO cook_state_snapshot(snapshotKey, snapshotValue, updateVersion, updateTime) "
		"VALUES(?, ?, ?, NOW()) "
		"ON DUPLICATE KEY UPDATE "
		"snapshotValue=IF(updateVersion<=VALUES(updateVersion), VALUES(snapshotValue), snapshotValue), "
		"updateTime=IF(updateVersion<=VALUES(updateVersion), VALUES(updateTime), updateTime), "
		"updateVersion=GREATEST(updateVersion, VALUES(updateVersion))");
	if (statement == nullptr || !statement->isValid())
	{
		LOGERROR("CookStorageService prepare mysql save snapshot failed error=%s", mysql->lastError().c_str());
		return false;
	}
	statement->setString(0, mysqlSnapshotKey());
	std::istringstream blobStream(stateText);
	statement->setBlob(1, &blobStream);
	statement->setInt64(2, version);
	MysqlCppExecuteResult result = mysql->executeUpdate(statement, true);
	if (!result.m_success)
	{
		LOGERROR("CookStorageService mysql save snapshot failed version=%lld bytes=%d error=%s",
		         static_cast<long long>(version),
		         static_cast<int32_t>(stateText.size()),
		         result.m_errorMessage.c_str());
		return false;
	}
	LOGDEBUG("CookStorageService mysql snapshot synced version=%lld bytes=%d affectedRows=%d",
	         static_cast<long long>(version),
	         static_cast<int32_t>(stateText.size()),
	         result.m_affectedRows);
	return true;
}

bool CookStorageService::executeMysqlUpdate(const std::shared_ptr<MysqlCpp>& mysql, const std::string& sql) const
{
	if (mysql == nullptr)
	{
		return false;
	}
	std::shared_ptr<MysqlCppPrepareStatement> statement = mysql->prepare(sql);
	if (statement == nullptr || !statement->isValid())
	{
		LOGERROR("CookStorageService prepare mysql update failed sql=%s error=%s", sql.c_str(), mysql->lastError().c_str());
		return false;
	}
	MysqlCppExecuteResult result = mysql->executeUpdate(statement, true);
	if (!result.m_success)
	{
		LOGERROR("CookStorageService execute mysql update failed sql=%s error=%s", sql.c_str(), result.m_errorMessage.c_str());
		return false;
	}
	return true;
}

std::shared_ptr<MysqlCpp> CookStorageService::acquireMysql()
{
	std::shared_ptr<MysqlCpp> mysql = m_mysqlPool.acquire(g_config.m_mysqlAcquireTimeoutMilliseconds);
	if (mysql == nullptr)
	{
		LOGERROR("CookStorageService acquire mysql failed timeoutMs=%d error=%s",
		         g_config.m_mysqlAcquireTimeoutMilliseconds,
		         m_mysqlPool.lastError().c_str());
	}
	return mysql;
}

std::shared_ptr<HiRedis> CookStorageService::acquireRedis()
{
	std::shared_ptr<HiRedis> redis = m_redisPool.acquire(g_config.m_redisCommandTimeoutMilliseconds);
	if (redis == nullptr)
	{
		LOGERROR("CookStorageService acquire redis failed timeoutMs=%d error=%s",
		         g_config.m_redisCommandTimeoutMilliseconds,
		         m_redisPool.lastError().c_str());
	}
	return redis;
}

MysqlCppConfig CookStorageService::mysqlConfig(bool withDatabase) const
{
	MysqlCppConfig config;
	config.m_ip = g_config.m_mysqlHost;
	config.m_port = g_config.m_mysqlPort;
	config.m_account = g_config.m_mysqlUser;
	config.m_password = g_config.m_mysqlPassword;
	config.m_dbName = withDatabase ? g_config.m_mysqlDatabase : "";
	config.m_autoCommit = false;
	config.m_autoReconnect = true;
	config.m_connectTimeoutSeconds = g_config.m_mysqlConnectTimeoutSeconds;
	config.m_readTimeoutSeconds = g_config.m_mysqlReadTimeoutSeconds;
	config.m_writeTimeoutSeconds = g_config.m_mysqlWriteTimeoutSeconds;
	return config;
}

HiRedisConfig CookStorageService::redisConfig() const
{
	HiRedisConfig config;
	config.m_ip = g_config.m_redisHost;
	config.m_port = g_config.m_redisPort;
	config.m_password = g_config.m_redisPassword;
	config.m_dbIndex = g_config.m_redisDbIndex;
	config.m_connectTimeoutMs = g_config.m_redisConnectTimeoutMilliseconds;
	config.m_commandTimeoutMs = g_config.m_redisCommandTimeoutMilliseconds;
	config.m_enableKeepAlive = g_config.m_redisEnableKeepAlive;
	return config;
}

bool CookStorageService::isMysqlIdentifierValid(const std::string& identifier) const
{
	if (identifier.empty())
	{
		return false;
	}
	for (size_t i = 0; i < identifier.size(); ++i)
	{
		unsigned char ch = static_cast<unsigned char>(identifier[i]);
		if (!(std::isalnum(ch) || ch == '_'))
		{
			return false;
		}
	}
	return true;
}

std::string CookStorageService::redisSnapshotKey() const
{
	return redisKeyPrefix() + ":account_store:snapshot";
}

std::string CookStorageService::redisSnapshotVersionKey() const
{
	return redisKeyPrefix() + ":account_store:version";
}

std::string CookStorageService::redisKeyPrefix() const
{
	std::string prefix = g_config.m_redisKeyPrefix.empty() ? "cook" : g_config.m_redisKeyPrefix;
	while (!prefix.empty() && prefix[prefix.size() - 1] == ':')
	{
		prefix.erase(prefix.size() - 1);
	}
	return prefix.empty() ? "cook" : prefix;
}

std::string CookStorageService::mysqlSnapshotKey() const
{
	return "account_store";
}
