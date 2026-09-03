#include "PdfReaderInstance.h"

#include <CSystem/CSystemAPI.h>
#include <LogManager/LogManagerAPI.h>

#include <stdio.h>

PdfReaderInstance::PdfReaderInstance() :
m_debugLogEnabled(false)
{
}

PdfReaderInstance& PdfReaderInstance::instance()
{
    static PdfReaderInstance s_instance;
    return s_instance;
}

void PdfReaderInstance::setDebugLogEnabled(bool enabled)
{
    if (m_debugLogEnabled == enabled)
    {
        return;
    }

    if (enabled)
    {
        m_debugLogEnabled = true;

        LogManagerConfig config;
        config.m_fileId = 0;
        config.m_path = "";
        config.m_maxFileBytes = 20LL * 1024LL * 1024LL;
        config.m_maxFileCount = 8;
        config.m_checkFileSizeInterval = 20;
        config.m_outputConsole = false;
        config.m_archiveOldLog = true;
        LogManager::instance().init(config);

        logInfo("PdfReader debug log enabled, pid=%u, exePath=%s",
                static_cast<unsigned int>(CSystem::currentProcessPid()),
                CSystem::GetCurrentExePath().c_str());
    }
    else
    {
        uninitDebugLog();
    }
}

bool PdfReaderInstance::isDebugLogEnabled() const
{
    return m_debugLogEnabled;
}

void PdfReaderInstance::uninitDebugLog()
{
    if (!m_debugLogEnabled)
    {
        return;
    }

    logInfo("PdfReader debug log closing");
    LogManager::instance().uninitAll();
    m_debugLogEnabled = false;
}

void PdfReaderInstance::logInfo(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    logText(LogManager::LOG_INFO, format, args);
    va_end(args);
}

void PdfReaderInstance::logWarning(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    logText(LogManager::LOG_WARNING, format, args);
    va_end(args);
}

void PdfReaderInstance::logError(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    logText(LogManager::LOG_ERROR, format, args);
    va_end(args);
}

void PdfReaderInstance::logFatal(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    logText(LogManager::LOG_FATAL, format, args);
    va_end(args);
}

void PdfReaderInstance::logText(int level, const char* format, va_list args)
{
    if (!m_debugLogEnabled || format == nullptr)
    {
        return;
    }

#ifdef _WIN32
    va_list argsCopy = args;
    int size = _vscprintf(format, argsCopy);
#else
    va_list argsCopy;
    va_copy(argsCopy, args);
    int size = vsnprintf(nullptr, 0, format, argsCopy);
    va_end(argsCopy);
#endif
    if (size < 0)
    {
        return;
    }

    std::vector<char> buffer(static_cast<size_t>(size) + 1);
#ifdef _WIN32
    int writeSize = vsprintf_s(&buffer[0], buffer.size(), format, args);
#else
    int writeSize = vsnprintf(&buffer[0], buffer.size(), format, args);
#endif
    if (writeSize < 0)
    {
        return;
    }

    LogManager::instance().print(0,
                                 static_cast<LogManager::LogLevel>(level),
                                 "",
                                 "",
                                 "",
                                 "",
                                 0,
                                 "%s",
                                 &buffer[0]);
}

LONG WINAPI PdfReaderInstance::unhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo)
{
    DWORD code = 0;
    void* address = nullptr;
    if (exceptionInfo != nullptr && exceptionInfo->ExceptionRecord != nullptr)
    {
        code = exceptionInfo->ExceptionRecord->ExceptionCode;
        address = exceptionInfo->ExceptionRecord->ExceptionAddress;
    }

    PdfReaderInstance::instance().logFatal("Unhandled exception, code=0x%08X, address=%p",
                                           static_cast<unsigned int>(code),
                                           address);
    if (PdfReaderInstance::instance().isDebugLogEnabled())
    {
        LogManager::instance().uninitAll();
    }
    return EXCEPTION_EXECUTE_HANDLER;
}