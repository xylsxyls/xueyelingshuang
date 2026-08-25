#include "Config.h"
#include "Cini/CiniAPI.h"
#include "CSystem/CSystemAPI.h"
#include <cctype>
#include <cstdlib>

Config& Config::instance()
{
	static Config s_config;
	return s_config;
}

Config::Config() :
m_httpPort(0),
m_httpBacklog(0),
m_httpWorkerThreads(0),
m_maxRequestBytes(0),
m_enableCors(false),
m_mysqlPort(0),
m_mysqlPoolSize(0),
m_mysqlAcquireTimeoutMilliseconds(0),
m_mysqlConnectTimeoutSeconds(0),
m_mysqlReadTimeoutSeconds(0),
m_mysqlWriteTimeoutSeconds(0),
m_redisPort(0),
m_redisDbIndex(0),
m_redisPoolSize(0),
m_redisConnectTimeoutMilliseconds(0),
m_redisCommandTimeoutMilliseconds(0),
m_redisEnableKeepAlive(false),
m_dependencyAutoStart(false),
m_dependencyWaitTimeoutMilliseconds(0),
m_dependencyPollIntervalMilliseconds(0),
m_dependencyStartupSettleMilliseconds(0),
m_mysqlAutoStart(false),
m_redisAutoStart(false),
m_storageSyncThreadCount(0),
m_storageSyncTaskPriority(0),
m_accountDefaultCoins(0),
m_accountDefaultYuanbao(0),
m_accountMaxRechargeCoins(0),
m_authTokenExpireSeconds(0),
m_checkinRewardCoins(0),
m_adRewardCoins(0),
m_adRequiredSeconds(0),
m_maxVideoBytes(0),
m_videoResourceScanIntervalSeconds(0),
m_videoFeedDefaultCount(0),
m_videoSeenExpireDays(0),
m_videoStreamChunkBytes(0),
m_videoPosterGenerateEnabled(false),
m_videoPosterCommandUseShell(false),
m_videoPosterMaxWidth(0),
m_videoPosterMaxHeight(0),
m_videoPosterJpegQuality(0),
m_videoPosterTimeoutMilliseconds(0),
m_videoPosterApplyRotation(false),
m_voiceMaxTextBytes(0),
m_voiceInlineAudioMaxBytes(0),
m_voiceWorkerThreadCount(0),
m_voiceCommandUseShell(false),
m_voiceInitialRequiredCount(0),
m_voiceInitialPromptGroupCount(0),
m_voiceClientConcurrency(0),
m_voiceFetchWaitMilliseconds(0),
m_voiceFetchMaxAttempts(0),
m_mainLoopSleepMilliseconds(0),
m_freeBlockMinimumSeconds(0),
m_logMaxFileBytes(0),
m_logMaxFileCount(0),
m_logCheckFileSizeInterval(0),
m_logOutputConsole(false),
m_logArchiveOldLog(false),
m_isInit(false)
{

}

Config::~Config()
{

}

bool Config::init()
{
	if (m_isInit)
	{
		return true;
	}
	setDefaults();
	loadIniOverrides();
	m_isInit = true;
	return true;
}

void Config::setDefaults()
{
	m_configFileName = "CookServer.ini";
	m_serverPublicIp = "175.24.198.162";
	m_httpListenHost = "0.0.0.0";
	m_httpPort = 5207;
	m_apiVersion = "cook-phase1-0.2.0";
	m_httpBacklog = 128;
	m_httpWorkerThreads = 16;
	m_maxRequestBytes = 1024LL * 1024LL;
	m_enableCors = true;
	m_corsAllowOrigin = "*";
	m_storageBackend = "redis_mysql";
	m_mysqlHost = "127.0.0.1";
	m_mysqlPort = 3306;
	m_mysqlUser = "root";
	m_mysqlPassword = "";
	m_mysqlDatabase = "cook_server";
	m_mysqlPoolSize = 4;
	m_mysqlAcquireTimeoutMilliseconds = 3000;
	m_mysqlConnectTimeoutSeconds = 5;
	m_mysqlReadTimeoutSeconds = 10;
	m_mysqlWriteTimeoutSeconds = 10;
	m_redisHost = "127.0.0.1";
	m_redisPort = 6379;
	m_redisPassword = "";
	m_redisDbIndex = 0;
	m_redisPoolSize = 4;
	m_redisKeyPrefix = "cook";
	m_redisConnectTimeoutMilliseconds = 3000;
	m_redisCommandTimeoutMilliseconds = 3000;
	m_redisEnableKeepAlive = true;
	m_dependencyAutoStart = true;
	m_dependencyWaitTimeoutMilliseconds = 30000;
	m_dependencyPollIntervalMilliseconds = 500;
	m_dependencyStartupSettleMilliseconds = 3000;
	m_mysqlAutoStart = true;
	m_mysqlHome = "C:\\mysql_cook-5.6.24-win32";
	m_mysqlProcessName = "mysqld.exe";
	m_mysqlStartCommand = "\"{home}\\bin\\mysqld.exe\" --defaults-file=\"{home}\\my.ini\" --basedir=\"{home}\" --datadir=\"{home}\\data\" --console";
	m_mysqlWorkingDirectory = "{home}";
	m_redisAutoStart = true;
	m_redisHome = "C:\\redis_cook7.4.2";
	m_redisProcessName = "redis-server.exe";
	m_redisConfigFile = "{home}\\redis.conf";
	m_redisStartCommand = "\"{home}\\redis-server.exe\" \"{config}\"";
	m_redisWorkingDirectory = "{home}";
	m_storageSyncThreadCount = 1;
	m_storageSyncTaskPriority = 1;
	m_recipeResourceDirectoryName = "res/recipes";
	m_accountDefaultCoins = 0;
	m_accountDefaultYuanbao = 100;
	m_accountMaxRechargeCoins = 10000;
	m_authTokenExpireSeconds = 7 * 24 * 60 * 60;
	m_checkinRewardCoins = 5;
	m_adRewardCoins = 30;
	m_adRequiredSeconds = 30;
	m_maxVideoBytes = 100LL * 1024LL * 1024LL;
	m_videoStorageDirectoryName = "data/videos/files";
	m_videoResourceDirectoryName = "res/video";
	m_videoResourceScanIntervalSeconds = 30;
	m_videoFeedDefaultCount = 2;
	m_videoSeenExpireDays = 1;
	m_videoStreamChunkBytes = 4LL * 1024LL * 1024LL;
	m_videoPosterApiPath = "/api/videos/poster";
	m_videoPosterGenerateEnabled = true;
	m_videoPosterCommandTemplate = "";
	m_videoPosterCommandUseShell = false;
	m_videoPosterMaxWidth = 720;
	m_videoPosterMaxHeight = 1280;
	m_videoPosterJpegQuality = 82;
	m_videoPosterTimeoutMilliseconds = 10000;
	m_videoPosterApplyRotation = true;
	m_videoPosterPrefetchCount = 5;
	m_videoPosterMemoryCacheMaxItems = 100;
	m_voiceTtsProvider = "windows_sapi";
	m_voiceCacheDirectoryName = "data/voice/cache";
	m_voiceAudioApiPath = "/api/voice/audio";
	m_voiceMaxTextBytes = 2048;
	m_voiceInlineAudioMaxBytes = 256 * 1024;
	m_voiceWorkerThreadCount = 8;
	m_voiceCommandTemplate = "";
	m_voiceCommandAudioFormat = "mp3";
	m_voiceCommandUseShell = false;
	m_voiceInitialRequiredCount = 2;
	m_voiceInitialPromptGroupCount = 2;
	m_voiceClientConcurrency = 2;
	m_voiceFetchWaitMilliseconds = 30000;
	m_voiceFetchMaxAttempts = 3;
	m_mainLoopSleepMilliseconds = 500;
	m_freeBlockMinimumSeconds = 120;
	m_logPath = "";
	m_logMaxFileBytes = 100LL * 1024LL * 1024LL;
	m_logMaxFileCount = 5;
	m_logCheckFileSizeInterval = 100;
	m_logOutputConsole = false;
	m_logArchiveOldLog = true;
}

void Config::loadIniOverrides()
{
	std::string path = iniPath();
	if (!CSystem::DirOrFileExist(path))
	{
		return;
	}

	Cini ini(path, true);
	m_serverPublicIp = readIniString(ini, "Server", "PublicIp", m_serverPublicIp);
	m_httpListenHost = readIniString(ini, "Server", "ListenHost", m_httpListenHost);
	int32_t httpPort = readIniInt32(ini, "Server", "Port", static_cast<int32_t>(m_httpPort));
	if (httpPort > 0 && httpPort <= 65535)
	{
		m_httpPort = static_cast<uint16_t>(httpPort);
	}
	m_apiVersion = readIniString(ini, "Server", "ApiVersion", m_apiVersion);
	m_httpBacklog = readIniInt32(ini, "Server", "Backlog", m_httpBacklog);
	m_httpWorkerThreads = readIniInt32(ini, "Server", "WorkerThreads", m_httpWorkerThreads);
	m_maxRequestBytes = readIniInt64(ini, "Server", "MaxRequestBytes", m_maxRequestBytes);
	m_enableCors = readIniBool(ini, "Server", "EnableCors", m_enableCors);
	m_corsAllowOrigin = readIniString(ini, "Server", "CorsAllowOrigin", m_corsAllowOrigin);

	m_storageBackend = readIniString(ini, "Storage", "Backend", m_storageBackend);
	m_recipeResourceDirectoryName = readIniString(ini, "Storage", "RecipeResourceDirectoryName", m_recipeResourceDirectoryName);
	m_videoStorageDirectoryName = readIniString(ini, "Storage", "VideoStorageDirectoryName", m_videoStorageDirectoryName);
	m_videoResourceDirectoryName = readIniString(ini, "Storage", "VideoResourceDirectoryName", m_videoResourceDirectoryName);
	m_videoResourceScanIntervalSeconds = readIniInt32(ini, "Storage", "VideoResourceScanIntervalSeconds", m_videoResourceScanIntervalSeconds);
	if (m_videoResourceScanIntervalSeconds < 0)
	{
		m_videoResourceScanIntervalSeconds = 0;
	}
	m_storageSyncThreadCount = readIniInt32(ini, "Storage", "SyncThreadCount", m_storageSyncThreadCount);
	if (m_storageSyncThreadCount < 1)
	{
		m_storageSyncThreadCount = 1;
	}
	m_storageSyncTaskPriority = readIniInt32(ini, "Storage", "SyncTaskPriority", m_storageSyncTaskPriority);
	if (m_storageSyncTaskPriority < 1)
	{
		m_storageSyncTaskPriority = 1;
	}

	m_mysqlHost = readIniString(ini, "MySQL", "Host", m_mysqlHost);
	m_mysqlPort = readIniInt32(ini, "MySQL", "Port", m_mysqlPort);
	if (m_mysqlPort <= 0 || m_mysqlPort > 65535)
	{
		m_mysqlPort = 3306;
	}
	m_mysqlUser = readIniString(ini, "MySQL", "User", m_mysqlUser);
	m_mysqlPassword = readIniString(ini, "MySQL", "Password", m_mysqlPassword);
	m_mysqlDatabase = readIniString(ini, "MySQL", "Database", m_mysqlDatabase);
	m_mysqlPoolSize = readIniInt32(ini, "MySQL", "PoolSize", m_mysqlPoolSize);
	if (m_mysqlPoolSize < 1)
	{
		m_mysqlPoolSize = 1;
	}
	m_mysqlAcquireTimeoutMilliseconds = readIniInt32(ini, "MySQL", "AcquireTimeoutMilliseconds", m_mysqlAcquireTimeoutMilliseconds);
	if (m_mysqlAcquireTimeoutMilliseconds < 100)
	{
		m_mysqlAcquireTimeoutMilliseconds = 100;
	}
	m_mysqlConnectTimeoutSeconds = readIniInt32(ini, "MySQL", "ConnectTimeoutSeconds", m_mysqlConnectTimeoutSeconds);
	if (m_mysqlConnectTimeoutSeconds < 1)
	{
		m_mysqlConnectTimeoutSeconds = 1;
	}
	m_mysqlReadTimeoutSeconds = readIniInt32(ini, "MySQL", "ReadTimeoutSeconds", m_mysqlReadTimeoutSeconds);
	if (m_mysqlReadTimeoutSeconds < 1)
	{
		m_mysqlReadTimeoutSeconds = 1;
	}
	m_mysqlWriteTimeoutSeconds = readIniInt32(ini, "MySQL", "WriteTimeoutSeconds", m_mysqlWriteTimeoutSeconds);
	if (m_mysqlWriteTimeoutSeconds < 1)
	{
		m_mysqlWriteTimeoutSeconds = 1;
	}

	m_redisHost = readIniString(ini, "Redis", "Host", m_redisHost);
	m_redisPort = readIniInt32(ini, "Redis", "Port", m_redisPort);
	if (m_redisPort <= 0 || m_redisPort > 65535)
	{
		m_redisPort = 6379;
	}
	m_redisPassword = readIniString(ini, "Redis", "Password", m_redisPassword);
	m_redisDbIndex = readIniInt32(ini, "Redis", "DbIndex", m_redisDbIndex);
	if (m_redisDbIndex < 0)
	{
		m_redisDbIndex = 0;
	}
	m_redisPoolSize = readIniInt32(ini, "Redis", "PoolSize", m_redisPoolSize);
	if (m_redisPoolSize < 1)
	{
		m_redisPoolSize = 1;
	}
	m_redisKeyPrefix = readIniString(ini, "Redis", "KeyPrefix", m_redisKeyPrefix);
	m_redisConnectTimeoutMilliseconds = readIniInt32(ini, "Redis", "ConnectTimeoutMilliseconds", m_redisConnectTimeoutMilliseconds);
	if (m_redisConnectTimeoutMilliseconds < 100)
	{
		m_redisConnectTimeoutMilliseconds = 100;
	}
	m_redisCommandTimeoutMilliseconds = readIniInt32(ini, "Redis", "CommandTimeoutMilliseconds", m_redisCommandTimeoutMilliseconds);
	if (m_redisCommandTimeoutMilliseconds < 100)
	{
		m_redisCommandTimeoutMilliseconds = 100;
	}
	m_redisEnableKeepAlive = readIniBool(ini, "Redis", "EnableKeepAlive", m_redisEnableKeepAlive);

	m_dependencyAutoStart = readIniBool(ini, "Dependencies", "AutoStart", m_dependencyAutoStart);
	m_dependencyWaitTimeoutMilliseconds = readIniInt32(ini, "Dependencies", "WaitTimeoutMilliseconds", m_dependencyWaitTimeoutMilliseconds);
	if (m_dependencyWaitTimeoutMilliseconds < 1000)
	{
		m_dependencyWaitTimeoutMilliseconds = 1000;
	}
	m_dependencyPollIntervalMilliseconds = readIniInt32(ini, "Dependencies", "PollIntervalMilliseconds", m_dependencyPollIntervalMilliseconds);
	if (m_dependencyPollIntervalMilliseconds < 100)
	{
		m_dependencyPollIntervalMilliseconds = 100;
	}
	m_dependencyStartupSettleMilliseconds = readIniInt32(ini, "Dependencies", "StartupSettleMilliseconds", m_dependencyStartupSettleMilliseconds);
	if (m_dependencyStartupSettleMilliseconds < 0)
	{
		m_dependencyStartupSettleMilliseconds = 0;
	}
	m_mysqlAutoStart = readIniBool(ini, "MySQL", "AutoStart", m_mysqlAutoStart);
	m_mysqlHome = readIniString(ini, "MySQL", "Home", m_mysqlHome);
	m_mysqlProcessName = readIniString(ini, "MySQL", "ProcessName", m_mysqlProcessName);
	m_mysqlStartCommand = readIniString(ini, "MySQL", "StartCommand", m_mysqlStartCommand);
	m_mysqlWorkingDirectory = readIniString(ini, "MySQL", "WorkingDirectory", m_mysqlWorkingDirectory);
	m_redisAutoStart = readIniBool(ini, "Redis", "AutoStart", m_redisAutoStart);
	m_redisHome = readIniString(ini, "Redis", "Home", m_redisHome);
	m_redisProcessName = readIniString(ini, "Redis", "ProcessName", m_redisProcessName);
	m_redisConfigFile = readIniString(ini, "Redis", "ConfigFile", m_redisConfigFile);
	m_redisStartCommand = readIniString(ini, "Redis", "StartCommand", m_redisStartCommand);
	m_redisWorkingDirectory = readIniString(ini, "Redis", "WorkingDirectory", m_redisWorkingDirectory);

	m_accountDefaultCoins = readIniInt32(ini, "Account", "DefaultCoins", m_accountDefaultCoins);
	m_accountDefaultYuanbao = readIniInt32(ini, "Account", "DefaultYuanbao", m_accountDefaultYuanbao);
	m_accountMaxRechargeCoins = readIniInt32(ini, "Account", "MaxRechargeCoins", m_accountMaxRechargeCoins);
	m_authTokenExpireSeconds = readIniInt32(ini, "Account", "AuthTokenExpireSeconds", m_authTokenExpireSeconds);
	m_checkinRewardCoins = readIniInt32(ini, "Account", "CheckinRewardCoins", m_checkinRewardCoins);
	m_adRewardCoins = readIniInt32(ini, "Account", "AdRewardCoins", m_adRewardCoins);
	m_adRequiredSeconds = readIniInt32(ini, "Account", "AdRequiredSeconds", m_adRequiredSeconds);
	m_maxVideoBytes = readIniInt64(ini, "Account", "MaxVideoBytes", m_maxVideoBytes);

	m_videoFeedDefaultCount = readIniInt32(ini, "Feed", "DefaultCount", m_videoFeedDefaultCount);
	if (m_videoFeedDefaultCount < 1)
	{
		m_videoFeedDefaultCount = 1;
	}
	if (m_videoFeedDefaultCount > 20)
	{
		m_videoFeedDefaultCount = 20;
	}
	m_videoSeenExpireDays = readIniInt32(ini, "Feed", "VideoSeenExpireDays", m_videoSeenExpireDays);
	if (m_videoSeenExpireDays < 0)
	{
		m_videoSeenExpireDays = 0;
	}
	m_videoStreamChunkBytes = readIniInt64(ini, "Feed", "VideoStreamChunkBytes", m_videoStreamChunkBytes);
	if (m_videoStreamChunkBytes < 0)
	{
		m_videoStreamChunkBytes = 0;
	}
	m_videoPosterApiPath = readIniString(ini, "Feed", "VideoPosterApiPath", m_videoPosterApiPath);
	m_videoPosterGenerateEnabled = readIniBool(ini, "Feed", "VideoPosterGenerateEnabled", m_videoPosterGenerateEnabled);
	m_videoPosterCommandTemplate = readIniString(ini, "Feed", "VideoPosterCommandTemplate", m_videoPosterCommandTemplate);
	m_videoPosterCommandUseShell = readIniBool(ini, "Feed", "VideoPosterCommandUseShell", m_videoPosterCommandUseShell);
	m_videoPosterMaxWidth = readIniInt32(ini, "Feed", "VideoPosterMaxWidth", m_videoPosterMaxWidth);
	if (m_videoPosterMaxWidth < 0)
	{
		m_videoPosterMaxWidth = 0;
	}
	m_videoPosterMaxHeight = readIniInt32(ini, "Feed", "VideoPosterMaxHeight", m_videoPosterMaxHeight);
	if (m_videoPosterMaxHeight < 0)
	{
		m_videoPosterMaxHeight = 0;
	}
	m_videoPosterJpegQuality = readIniInt32(ini, "Feed", "VideoPosterJpegQuality", m_videoPosterJpegQuality);
	if (m_videoPosterJpegQuality < 1)
	{
		m_videoPosterJpegQuality = 1;
	}
	if (m_videoPosterJpegQuality > 100)
	{
		m_videoPosterJpegQuality = 100;
	}
	m_videoPosterTimeoutMilliseconds = readIniInt32(ini, "Feed", "VideoPosterTimeoutMilliseconds", m_videoPosterTimeoutMilliseconds);
	if (m_videoPosterTimeoutMilliseconds < 0)
	{
		m_videoPosterTimeoutMilliseconds = 0;
	}
	m_videoPosterApplyRotation = readIniBool(ini, "Feed", "VideoPosterApplyRotation", m_videoPosterApplyRotation);
	m_videoPosterPrefetchCount = readIniInt32(ini, "Feed", "VideoPosterPrefetchCount", m_videoPosterPrefetchCount);
	if (m_videoPosterPrefetchCount < 0)
	{
		m_videoPosterPrefetchCount = 0;
	}
	if (m_videoPosterPrefetchCount > 20)
	{
		m_videoPosterPrefetchCount = 20;
	}
	m_videoPosterMemoryCacheMaxItems = readIniInt32(ini, "Feed", "VideoPosterMemoryCacheMaxItems", m_videoPosterMemoryCacheMaxItems);
	if (m_videoPosterMemoryCacheMaxItems < 0)
	{
		m_videoPosterMemoryCacheMaxItems = 0;
	}

	m_voiceTtsProvider = readIniString(ini, "Voice", "TtsProvider", m_voiceTtsProvider);
	m_voiceCacheDirectoryName = readIniString(ini, "Voice", "CacheDirectoryName", m_voiceCacheDirectoryName);
	m_voiceAudioApiPath = readIniString(ini, "Voice", "AudioApiPath", m_voiceAudioApiPath);
	m_voiceMaxTextBytes = readIniInt32(ini, "Voice", "MaxTextBytes", m_voiceMaxTextBytes);
	m_voiceInlineAudioMaxBytes = readIniInt32(ini, "Voice", "InlineAudioMaxBytes", m_voiceInlineAudioMaxBytes);
	m_voiceWorkerThreadCount = readIniInt32(ini, "Voice", "WorkerThreadCount", m_voiceWorkerThreadCount);
	if (m_voiceWorkerThreadCount < 1)
	{
		m_voiceWorkerThreadCount = 1;
	}
	m_voiceCommandTemplate = readIniString(ini, "Voice", "CommandTemplate", m_voiceCommandTemplate);
	m_voiceCommandAudioFormat = readIniString(ini, "Voice", "CommandAudioFormat", m_voiceCommandAudioFormat);
	m_voiceCommandUseShell = readIniBool(ini, "Voice", "CommandUseShell", m_voiceCommandUseShell);
	m_voiceInitialRequiredCount = readIniInt32(ini, "Voice", "InitialRequiredCount", m_voiceInitialRequiredCount);
	if (m_voiceInitialRequiredCount < 1)
	{
		m_voiceInitialRequiredCount = 1;
	}
	m_voiceInitialPromptGroupCount = readIniInt32(ini, "Voice", "InitialPromptGroupCount", m_voiceInitialRequiredCount);
	if (m_voiceInitialPromptGroupCount < 1)
	{
		m_voiceInitialPromptGroupCount = 1;
	}
	m_voiceClientConcurrency = readIniInt32(ini, "Voice", "ClientConcurrency", m_voiceClientConcurrency);
	if (m_voiceClientConcurrency < 1)
	{
		m_voiceClientConcurrency = 1;
	}
	m_voiceFetchWaitMilliseconds = readIniInt32(ini, "Voice", "FetchWaitMilliseconds", m_voiceFetchWaitMilliseconds);
	if (m_voiceFetchWaitMilliseconds < 1000)
	{
		m_voiceFetchWaitMilliseconds = 1000;
	}
	m_voiceFetchMaxAttempts = readIniInt32(ini, "Voice", "FetchMaxAttempts", m_voiceFetchMaxAttempts);
	if (m_voiceFetchMaxAttempts < 1)
	{
		m_voiceFetchMaxAttempts = 1;
	}

	m_mainLoopSleepMilliseconds = readIniInt32(ini, "Runtime", "MainLoopSleepMilliseconds", m_mainLoopSleepMilliseconds);
	m_freeBlockMinimumSeconds = readIniInt32(ini, "Runtime", "FreeBlockMinimumSeconds", m_freeBlockMinimumSeconds);
	if (m_freeBlockMinimumSeconds < 0)
	{
		m_freeBlockMinimumSeconds = 0;
	}

	m_logPath = readIniString(ini, "Log", "Path", m_logPath);
	m_logMaxFileBytes = readIniInt64(ini, "Log", "MaxFileBytes", m_logMaxFileBytes);
	m_logMaxFileCount = readIniInt32(ini, "Log", "MaxFileCount", m_logMaxFileCount);
	m_logCheckFileSizeInterval = readIniInt32(ini, "Log", "CheckFileSizeInterval", m_logCheckFileSizeInterval);
	m_logOutputConsole = readIniBool(ini, "Log", "OutputConsole", m_logOutputConsole);
	m_logArchiveOldLog = readIniBool(ini, "Log", "ArchiveOldLog", m_logArchiveOldLog);
}

std::string Config::iniPath() const
{
	return CSystem::GetCurrentExePath() + m_configFileName;
}

std::string Config::readIniString(const Cini& ini, const std::string& section, const std::string& key, const std::string& defaultValue) const
{
	std::string value = normalizeIniText(ini.readIni(key, section));
	return value.empty() ? defaultValue : value;
}

int32_t Config::readIniInt32(const Cini& ini, const std::string& section, const std::string& key, int32_t defaultValue) const
{
	std::string value = normalizeIniText(ini.readIni(key, section));
	if (value.empty())
	{
		return defaultValue;
	}
	char* endPtr = nullptr;
	long number = std::strtol(value.c_str(), &endPtr, 10);
	return (endPtr != nullptr && *endPtr == '\0') ? static_cast<int32_t>(number) : defaultValue;
}

int64_t Config::readIniInt64(const Cini& ini, const std::string& section, const std::string& key, int64_t defaultValue) const
{
	std::string value = normalizeIniText(ini.readIni(key, section));
	if (value.empty())
	{
		return defaultValue;
	}
	char* endPtr = nullptr;
	long long number = 0;
#ifdef _WIN32
	number = _strtoi64(value.c_str(), &endPtr, 10);
#else
	number = std::strtoll(value.c_str(), &endPtr, 10);
#endif
	return (endPtr != nullptr && *endPtr == '\0') ? static_cast<int64_t>(number) : defaultValue;
}

bool Config::readIniBool(const Cini& ini, const std::string& section, const std::string& key, bool defaultValue) const
{
	std::string value = normalizeIniText(ini.readIni(key, section));
	if (value.empty())
	{
		return defaultValue;
	}
	for (size_t i = 0; i < value.size(); ++i)
	{
		value[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(value[i])));
	}
	if (value == "1" || value == "true" || value == "yes" || value == "on")
	{
		return true;
	}
	if (value == "0" || value == "false" || value == "no" || value == "off")
	{
		return false;
	}
	return defaultValue;
}

std::string Config::normalizeIniText(const std::string& text) const
{
	size_t begin = 0;
	while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])))
	{
		++begin;
	}
	size_t end = text.size();
	while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])))
	{
		--end;
	}
	return text.substr(begin, end - begin);
}

void Config::uninit()
{
	m_configFileName.clear();
	m_serverPublicIp.clear();
	m_httpListenHost.clear();
	m_httpPort = 0;
	m_apiVersion.clear();
	m_httpBacklog = 0;
	m_httpWorkerThreads = 0;
	m_maxRequestBytes = 0;
	m_enableCors = false;
	m_corsAllowOrigin.clear();
	m_storageBackend.clear();
	m_mysqlHost.clear();
	m_mysqlPort = 0;
	m_mysqlUser.clear();
	m_mysqlPassword.clear();
	m_mysqlDatabase.clear();
	m_mysqlPoolSize = 0;
	m_mysqlAcquireTimeoutMilliseconds = 0;
	m_mysqlConnectTimeoutSeconds = 0;
	m_mysqlReadTimeoutSeconds = 0;
	m_mysqlWriteTimeoutSeconds = 0;
	m_redisHost.clear();
	m_redisPort = 0;
	m_redisPassword.clear();
	m_redisDbIndex = 0;
	m_redisPoolSize = 0;
	m_redisKeyPrefix.clear();
	m_redisConnectTimeoutMilliseconds = 0;
	m_redisCommandTimeoutMilliseconds = 0;
	m_redisEnableKeepAlive = false;
	m_dependencyAutoStart = false;
	m_dependencyWaitTimeoutMilliseconds = 0;
	m_dependencyPollIntervalMilliseconds = 0;
	m_dependencyStartupSettleMilliseconds = 0;
	m_mysqlAutoStart = false;
	m_mysqlHome.clear();
	m_mysqlProcessName.clear();
	m_mysqlStartCommand.clear();
	m_mysqlWorkingDirectory.clear();
	m_redisAutoStart = false;
	m_redisHome.clear();
	m_redisProcessName.clear();
	m_redisConfigFile.clear();
	m_redisStartCommand.clear();
	m_redisWorkingDirectory.clear();
	m_storageSyncThreadCount = 0;
	m_storageSyncTaskPriority = 0;
	m_recipeResourceDirectoryName.clear();
	m_accountDefaultCoins = 0;
	m_accountDefaultYuanbao = 0;
	m_accountMaxRechargeCoins = 0;
	m_authTokenExpireSeconds = 0;
	m_checkinRewardCoins = 0;
	m_adRewardCoins = 0;
	m_adRequiredSeconds = 0;
	m_maxVideoBytes = 0;
	m_videoStorageDirectoryName.clear();
	m_videoResourceDirectoryName.clear();
	m_videoResourceScanIntervalSeconds = 0;
	m_videoFeedDefaultCount = 0;
	m_videoSeenExpireDays = 0;
	m_videoStreamChunkBytes = 0;
	m_videoPosterApiPath.clear();
	m_videoPosterGenerateEnabled = false;
	m_videoPosterCommandTemplate.clear();
	m_videoPosterCommandUseShell = false;
	m_videoPosterMaxWidth = 0;
	m_videoPosterMaxHeight = 0;
	m_videoPosterJpegQuality = 0;
	m_videoPosterTimeoutMilliseconds = 0;
	m_videoPosterApplyRotation = false;
	m_videoPosterPrefetchCount = 0;
	m_videoPosterMemoryCacheMaxItems = 0;
	m_voiceTtsProvider.clear();
	m_voiceCacheDirectoryName.clear();
	m_voiceAudioApiPath.clear();
	m_voiceMaxTextBytes = 0;
	m_voiceInlineAudioMaxBytes = 0;
	m_voiceWorkerThreadCount = 0;
	m_voiceCommandTemplate.clear();
	m_voiceCommandAudioFormat.clear();
	m_voiceCommandUseShell = false;
	m_voiceInitialRequiredCount = 0;
	m_voiceInitialPromptGroupCount = 0;
	m_voiceClientConcurrency = 0;
	m_voiceFetchWaitMilliseconds = 0;
	m_voiceFetchMaxAttempts = 0;
	m_mainLoopSleepMilliseconds = 0;
	m_freeBlockMinimumSeconds = 0;
	m_logPath.clear();
	m_logMaxFileBytes = 0;
	m_logMaxFileCount = 0;
	m_logCheckFileSizeInterval = 0;
	m_logOutputConsole = false;
	m_logArchiveOldLog = false;
	m_isInit = false;
}