#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "Config.h"
#include "LumaPlayerLogger.h"

#include "LogManager/LogManagerAPI.h"

#include <stdarg.h>
#include <stdio.h>

void LumaPlayerLogger::init(bool enabled)
{
	if (g_config.m_logInitialized.load())
	{
		return;
	}
	LogManagerConfig config;
	config.m_fileId = 0;
	config.m_maxFileBytes = 20 * 1024 * 1024;
	config.m_maxFileCount = 8;
	config.m_checkFileSizeInterval = 1;
	config.m_outputConsole = false;
	config.m_archiveOldLog = false;
	LogManager::instance().set(true, true);
	LogManager::instance().init(config);
	g_config.m_logInitialized.store(true);
	log("LumaPlayer log enabled, debug=%d", enabled ? 1 : 0);
}

void LumaPlayerLogger::uninit()
{
	if (!g_config.m_logInitialized.load())
	{
		return;
	}
	log("LumaPlayer log closing");
	LogManager::instance().uninitAll();
	g_config.m_logInitialized.store(false);
}

bool LumaPlayerLogger::isEnabled()
{
	return g_config.m_logInitialized.load();
}

void LumaPlayerLogger::log(const char* format, ...)
{
	if (!g_config.m_logInitialized.load() || format == nullptr)
	{
		return;
	}
	char buffer[2048] = { 0 };
	va_list args;
	va_start(args, format);
#ifdef _MSC_VER
	_vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, format, args);
#else
	vsnprintf(buffer, sizeof(buffer), format, args);
#endif
	va_end(args);
	LogManager::instance().print(0, LogManager::LOG_INFO, __FILE__, __FUNCTION__, "", "", 0, "%s", buffer);
}