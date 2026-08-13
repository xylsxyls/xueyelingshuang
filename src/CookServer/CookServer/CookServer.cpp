#include "CookApiService.h"
#include "CookCatalog.h"
#include "CookHttpServer.h"
#include "Config.h"
#include "CDump/CDumpAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <stdint.h>
#include <atomic>
#include <csignal>
#include <iostream>

// 服务退出标记，由系统信号回调写入，主线程循环读取。
static std::atomic<bool> s_stopRequested(false);

/** 接收控制台停止信号，通知主循环退出
@param [in] signalValue 操作系统传入的信号值，当前不区分具体信号
*/
static void OnSignal(int32_t signalValue)
{
	(void)signalValue;
	s_stopRequested.store(true);
}

/** 判断服务是否已经收到退出请求
@return 返回true表示需要退出主循环
*/
static bool IsStopRequested()
{
	return s_stopRequested.load();
}

/** CookServer控制台程序入口
@return 返回0表示正常退出，返回非0表示启动失败
*/
int32_t main()
{
	static bool s_declareDumpFile = CDump::declareDumpFile();
	(void)s_declareDumpFile;

	if (!Config::instance().init())
	{
		std::cerr << "CookServer config init failed." << std::endl;
		return 1;
	}

	LogManagerConfig logConfig;
	logConfig.m_fileId = 0;
	logConfig.m_path = g_config.m_logPath;
	logConfig.m_maxFileBytes = g_config.m_logMaxFileBytes;
	logConfig.m_maxFileCount = g_config.m_logMaxFileCount;
	logConfig.m_checkFileSizeInterval = g_config.m_logCheckFileSizeInterval;
	logConfig.m_outputConsole = g_config.m_logOutputConsole;
	logConfig.m_archiveOldLog = g_config.m_logArchiveOldLog;
	LogManager::instance().init(logConfig);

	LOGINFO("CookServer startup begin pid=%d exePath=%s dumpDeclared=%d",
	        static_cast<int32_t>(CSystem::currentProcessPid()),
	        CSystem::GetCurrentExePath().c_str(),
	        s_declareDumpFile ? 1 : 0);
	std::string maxRequestBytesText = CStringManager::toStringInt64(g_config.m_maxRequestBytes);
	LOGINFO("CookServer config publicIp=%s listenHost=%s port=%d apiVersion=%s workerThreads=%d maxRequestBytes=%s",
	        g_config.m_serverPublicIp.c_str(),
	        g_config.m_httpListenHost.c_str(),
	        static_cast<int32_t>(g_config.m_httpPort),
	        g_config.m_apiVersion.c_str(),
	        g_config.m_httpWorkerThreads,
	        maxRequestBytesText.c_str());

	if (!CookCatalog::init())
	{
		LOGFATAL("CookServer recipe catalog init failed.");
		LogManager::instance().uninitAll();
		Config::instance().uninit();
		return 1;
	}
	LOGINFO("CookServer recipe catalog ready recipeCount=%d", static_cast<int32_t>(CookCatalog::recipeCatalog().size()));

	// 注册控制台退出信号，避免服务进程只能被强杀。
	std::signal(SIGINT, OnSignal);
	std::signal(SIGTERM, OnSignal);
	LOGINFO("CookServer signal handlers registered.");

	CookApiService apiService;
	CookHttpServer httpServer(&apiService);
	if (!httpServer.start())
	{
		LOGFATAL("CookServer http server start failed.");
		CookCatalog::uninit();
		LogManager::instance().uninitAll();
		Config::instance().uninit();
		return 1;
	}

	std::cout << "Press Ctrl+C to stop CookServer." << std::endl;
	LOGINFO("CookServer startup complete.");
	// 主线程只负责保持进程存活，HTTP工作线程由HttpService内部管理。
	while (!IsStopRequested())
	{
		CSystem::Sleep(g_config.m_mainLoopSleepMilliseconds);
	}
	LOGWARNING("CookServer stop signal received.");
	httpServer.stop();
	CookCatalog::uninit();
	LOGINFO("CookServer shutdown complete.");
	LogManager::instance().uninitAll();
	Config::instance().uninit();
	return 0;
}