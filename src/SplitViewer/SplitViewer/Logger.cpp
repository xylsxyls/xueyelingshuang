#include "Logger.h"

#include <cstdarg>
#include <string>

#include "CStringManager/CStringManagerAPI.h"
#include "LogManager/LogManagerAPI.h"

static bool g_splitViewerDebugLoggingEnabled = false;
static bool g_splitViewerLogManagerInitialized = false;

static std::string SplitViewerWideToLogString(const wchar_t* text)
{
    if (!text || !text[0])
    {
        return std::string();
    }

    return CStringManager::UnicodeToAnsi(std::wstring(text));
}

static void SplitViewerEnsureLogManagerInitialized()
{
    if (g_splitViewerLogManagerInitialized)
    {
        return;
    }

    LogManagerConfig config;
    config.m_fileId = 0;
    config.m_path = "";
    config.m_maxFileBytes = 20LL * 1024LL * 1024LL;
    config.m_maxFileCount = 5;
    config.m_checkFileSizeInterval = 25;
    config.m_outputConsole = false;
    config.m_archiveOldLog = true;

    LogManager::instance().set(true, false);
    LogManager::instance().init(config);
    g_splitViewerLogManagerInitialized = true;
    LOGINFO("%s", "SplitViewer debug logging initialized by command line argument.");
}

void SplitViewerSetDebugLoggingEnabled(bool enabled)
{
    if (enabled)
    {
        g_splitViewerDebugLoggingEnabled = true;
        SplitViewerEnsureLogManagerInitialized();
        return;
    }

    if (g_splitViewerLogManagerInitialized)
    {
        LOGINFO("%s", "SplitViewer debug logging disabled.");
        LogManager::instance().set(false, false);
        LogManager::instance().uninitAll();
        g_splitViewerLogManagerInitialized = false;
    }
    g_splitViewerDebugLoggingEnabled = false;
}

bool SplitViewerIsDebugLoggingEnabled()
{
    return g_splitViewerDebugLoggingEnabled;
}

void SplitViewerDebugLog(const wchar_t* message)
{
    if (!g_splitViewerDebugLoggingEnabled || !message)
    {
        return;
    }

    SplitViewerEnsureLogManagerInitialized();
    std::string text = SplitViewerWideToLogString(message);
    LOGINFO("%s", text.c_str());
}

void SplitViewerDebugLogFormat(const wchar_t* format, ...)
{
    if (!g_splitViewerDebugLoggingEnabled || !format)
    {
        return;
    }

    wchar_t buffer[2048] = { 0 };
    va_list args;
    va_start(args, format);
    StringCchVPrintfW(buffer, ARRAYSIZE(buffer), format, args);
    va_end(args);
    SplitViewerDebugLog(buffer);
}