#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "LumaPlayer.h"
#include "Config.h"
#include "CDump/CDumpAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include "LumaPlayerHelper.h"
#include "LumaPlayerLogger.h"

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
	LogManager::instance();
	CTaskThreadManager::Instance();
	int result = -1;
	try
	{
		QApplication application(argc, argv);
		g_config.init(application.arguments());
		LumaPlayerLogger::init(g_config.m_debugEnabled);
		LumaPlayerLogger::log("CDump registration result=%d", dumpEnabled ? 1 : 0);
		application.setFont(QFont(QString::fromWCharArray(L"楷体"), 10));
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
		LumaPlayerLogger::log("Unhandled std::exception in main: %s", exception.what());
		result = -2;
	}
	catch (...)
	{
		LumaPlayerLogger::log("Unhandled unknown exception in main");
		result = -3;
	}
	LumaPlayerLogger::uninit();
	g_config.uninit();
	return result;
}