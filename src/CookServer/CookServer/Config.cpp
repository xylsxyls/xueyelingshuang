#include "Config.h"

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
m_accountDefaultCoins(0),
m_accountMaxRechargeCoins(0),
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
	m_serverPublicIp = "175.24.198.162";
	m_httpListenHost = "0.0.0.0";
	m_httpPort = 5207;
	m_apiVersion = "cook-demo-0.1.0";
	m_httpBacklog = 128;
	m_httpWorkerThreads = 16;
	m_maxRequestBytes = 1024 * 1024;
	m_enableCors = true;
	m_corsAllowOrigin = "*";
	m_accountStateFileName = "CookServerState.txt";
	m_accountDefaultCoins = 60;
	m_accountMaxRechargeCoins = 10000;
	m_mainLoopSleepMilliseconds = 500;
	m_freeBlockMinimumSeconds = 600;
	m_logPath = "";
	m_logMaxFileBytes = 100LL * 1024LL * 1024LL;
	m_logMaxFileCount = 5;
	m_logCheckFileSizeInterval = 100;
	m_logOutputConsole = false;
	m_logArchiveOldLog = true;
	m_isInit = true;
	return true;
}

void Config::uninit()
{
	m_serverPublicIp.clear();
	m_httpListenHost.clear();
	m_httpPort = 0;
	m_apiVersion.clear();
	m_httpBacklog = 0;
	m_httpWorkerThreads = 0;
	m_maxRequestBytes = 0;
	m_enableCors = false;
	m_corsAllowOrigin.clear();
	m_accountStateFileName.clear();
	m_accountDefaultCoins = 0;
	m_accountMaxRechargeCoins = 0;
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