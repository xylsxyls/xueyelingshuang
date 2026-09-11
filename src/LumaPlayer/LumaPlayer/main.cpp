#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayer.h"
#include "Config.h"
#include "CDump/CDumpAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include "LumaPlayerHelper.h"
#include "LumaPlayerDialogSession.h"

#include <QApplication>
#include <QFont>

#include <algorithm>
#include <cctype>
#include <exception>
#include <string>

int main(int argc, char* argv[])
{
	// 在Qt和工作线程启动前注册崩溃转储，默认写入EXE目录
	const bool dumpEnabled = CDump::declareDumpFile();
	Config::instance();
	CTaskThreadManager::Instance();
	int result = -1;
	try
	{
		QApplication application(argc, argv);
		g_config.init(application.arguments());
        LogManagerConfig logConfig;
        logConfig.m_fileId = 0;
        logConfig.m_maxFileBytes = g_config.m_logMaxFileBytes;
        logConfig.m_maxFileCount = g_config.m_logMaxFileCount;
        logConfig.m_checkFileSizeInterval = 1;
        logConfig.m_outputConsole = false;
        logConfig.m_archiveOldLog = true;
        LogManager::instance().init(logConfig);
        g_config.m_logInitialized.store(true);
        LOGINFO("LumaPlayer startup, debug=%d", g_config.m_debugEnabled ? 1 : 0);
        LOGINFO("CDump registration result=%d", dumpEnabled ? 1 : 0);
        application.setFont(QFont(g_config.m_fontFamily, g_config.m_fontSize));
        LumaPlayerDialogSession dialogSession;
		const bool debugEnabled = g_config.m_debugEnabled;
		{
			LumaPlayer player(debugEnabled);
			player.show();
			player.loadMedia(g_config.m_startupMediaPath);
			result = application.exec();
		}
	}
	catch (const std::exception& exception)
	{
        LOGERROR("Unhandled std::exception in main: %s", exception.what());
		result = -2;
	}
	catch (...)
	{
        LOGERROR("Unhandled unknown exception in main");
		result = -3;
	}
    if (g_config.m_logInitialized.load())
    {
        LOGINFO("LumaPlayer log closing");
        LogManager::instance().uninit(0);
        g_config.m_logInitialized.store(false);
    }
	g_config.uninit();
	return result;
}