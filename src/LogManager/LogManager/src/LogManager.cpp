#include "LogManager.h"
#include "LogManagerFile.h"
#include "LogManagerHelper.h"
#include "CStringManager/CStringManagerAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "CSystem/CSystemAPI.h"
#include <cstdio>
#include <iostream>
#include <memory>
#include <new>
#include <stdarg.h>

LogManager::LogManager() :
#ifdef __EMSCRIPTEN__
m_exeName("wasm"), m_processName("wasm"),
#else
m_exeName(CSystem::GetCurrentExeFullName()),
m_processName(CSystem::GetCurrentExeName()),
#endif
m_logMap(),
m_writeBeginEnd(true),
m_writeLog(true),
m_activeWriters(0)
{
}

LogManager::~LogManager()
{
    uninitAll();
}

LogManager& LogManager::instance()
{
#if defined(_MSC_VER) && _MSC_VER < 1900
    static std::once_flag s_once;
    static LogManager* s_instance = nullptr;
    // VS2013 的局部静态初始化需要串行化，仅在构造成功后发布指针。
    std::call_once(s_once, []()
    {
        static LogManager s_manager;
        s_instance = &s_manager;
    });
    return *s_instance;
#else
    static LogManager s_manager;
    return s_manager;
#endif
}

bool LogManager::needPrintEvery(int64_t callIndex, int64_t interval)
{
    if (callIndex == 0)
    {
        return true;
    }
    if (interval < 0)
    {
        return false;
    }
    if (interval == 0)
    {
        return true;
    }
    return callIndex % interval == 0;
}

void LogManager::setDefaultConfig(const LogManagerConfig& config)
{
    std::lock_guard<std::mutex> lock(m_management);
    m_defaultConfig = config;
}

void LogManager::init()
{
    LogManagerRegistryGuard lock(m_management, m_activeWriters);
    if (m_writeLog)
    {
        initNoLock(m_defaultConfig);
    }
}

void LogManager::init(const LogManagerConfig& config)
{
    if (!m_writeLog)
    {
        return;
    }

    LogManagerRegistryGuard lock(m_management, m_activeWriters);
    initNoLock(config);
}

void LogManager::set(bool writeLog, bool writeBeginEnd)
{
    m_writeLog = writeLog;
    m_writeBeginEnd = writeBeginEnd;
}

void LogManager::print(int32_t fileId, LogLevel flag, const std::string& fileMacro, const std::string& funName, const std::string& exeName, const std::string& intDateTime, int32_t threadId, const char* format, ...)
{
    if (!m_writeLog)
    {
        return;
    }

    std::string message;
    va_list args;
    va_start(args, format);
    CStringManager::formatV(message, format, args);
    va_end(args);
    const std::string line = formatLineText(flag, fileMacro, funName, exeName, intDateTime, threadId, message);
    if (line.empty())
    {
        return;
    }

    {
        LogManagerWriterGuard lease(m_activeWriters);
        if (lease.acquired())
        {
            LogManagerFile* file = getLogFile(fileId);
            if (file && !file->m_rollRequested.load() && !file->m_config.m_outputConsole)
            {
                appendLine(file, line);
                return;
            }
            if (!file && fileId != 0)
            {
                return;
            }
        }
    }

    LogManagerRegistryGuard lock(m_management, m_activeWriters);
    LogManagerFile* logFile = getLogFile(fileId);
    if (logFile == nullptr && fileId == 0)
    {
        LogManagerConfig config = m_defaultConfig;
        config.m_fileId = 0;
        logFile = initNoLock(config);
    }
    if (logFile == nullptr)
    {
        return;
    }

    writeLineNoLock(logFile, line);
}

void LogManager::uninit(int32_t fileId)
{
    LogManagerRegistryGuard lock(m_management, m_activeWriters);
    uninitNoLock(fileId);
}

void LogManager::uninitAll()
{
    LogManagerRegistryGuard lock(m_management, m_activeWriters);
    while (!m_logMap.empty())
    {
        uninitNoLock(m_logMap.begin()->first);
    }
}

void LogManager::deleteFile(int32_t fileId)
{
    LogManagerRegistryGuard lock(m_management, m_activeWriters);
    LogManagerFile* file = getLogFile(fileId);
    if (file == nullptr)
    {
        return;
    }
    file->closeEntity();
    LogManagerHelper::closeFileLock(file->m_linkFileLock);
    if (file->m_linkIndex >= 0)
    {
        LogManagerHelper::deleteFile(file->m_linkPath);
    }
    for (const std::string& path : file->m_entityPaths)
    {
        LogManagerHelper::deleteFile(path);
    }
    m_logMap.erase(fileId);
    delete file;
}

int32_t LogManager::findFileId(const std::string& path)
{
    const std::string lookup = CSystem::absolutePath(path);
    const std::string directory = lookup.empty() ? "" : LogManagerHelper::normalizeLogDir(lookup);
    std::lock_guard<std::mutex> lock(m_management);
    for (const auto& entry : m_logMap)
    {
        const LogManagerFile* file = entry.second;
        if ((path.empty() && file->m_config.m_path.empty()) || file->m_logDir == directory ||
            (file->m_linkIndex == file->m_currentIndex && file->m_linkPath == lookup) || file->m_currentEntityPath == lookup)
        {
            return entry.first;
        }
    }
    return -1;
}

LogManagerFile* LogManager::getLogFile(int32_t fileId)
{
    std::map<int32_t, LogManagerFile*>::iterator it = m_logMap.find(fileId);
    if (it == m_logMap.end())
    {
        return nullptr;
    }
    return it->second;
}

LogManagerFile* LogManager::initNoLock(const LogManagerConfig& config)
{
    if (config.m_fileId == -1 || config.m_path.find('\0') != std::string::npos)
    {
        std::fputs("LogManager: invalid file ID or path\n", stderr);
        return nullptr;
    }
    LogManagerFile* oldLogFile = getLogFile(config.m_fileId);
    if (oldLogFile != nullptr)
    {
        return oldLogFile;
    }

    LogManagerConfig realConfig = config;
    realConfig.m_maxFileBytes = LogManagerHelper::normalizeMaxFileBytes(realConfig.m_maxFileBytes);
    realConfig.m_maxFileCount = LogManagerHelper::normalizeMaxFileCount(realConfig.m_maxFileCount);
    realConfig.m_checkFileSizeInterval = LogManagerHelper::normalizeCheckFileSizeInterval(realConfig.m_checkFileSizeInterval);

    const std::string configured = LogManagerHelper::normalizeLogDir(realConfig.m_path);
    const std::string absolute = CSystem::absolutePath(configured);
    if (absolute.empty() || !LogManagerHelper::createDirectoryRecursive(absolute))
    {
        std::fprintf(stderr, "LogManager: cannot create log directory %s\n", configured.c_str());
        return nullptr;
    }

    std::unique_ptr<LogManagerFile> owner(new (std::nothrow) LogManagerFile);
    LogManagerFile* logFile = owner.get();
    if (logFile == nullptr)
    {
        return nullptr;
    }
    logFile->m_config = realConfig;
    logFile->m_logDir = LogManagerHelper::normalizeLogDir(absolute);
    if (!openSessionNoLock(logFile))
    {
        return nullptr;
    }
    updateLinkNoLock(logFile);
    m_logMap[realConfig.m_fileId] = logFile;
    owner.release();

    if (m_writeBeginEnd)
    {
        std::string line = formatLineText(LOG_BEGIN, __FILE__, __FUNCTION__, "", "", 0, "");
        writeLineNoLock(logFile, line);
    }
    return logFile;
}

void LogManager::uninitNoLock(int32_t fileId)
{
    std::map<int32_t, LogManagerFile*>::iterator it = m_logMap.find(fileId);
    if (it == m_logMap.end())
    {
        return;
    }

    LogManagerFile* logFile = it->second;
    if (m_writeLog && m_writeBeginEnd)
    {
        std::string line = formatLineText(LOG_END, __FILE__, __FUNCTION__, "", "", 0, "");
        writeLineNoLock(logFile, line);
    }
    if (logFile->m_linkIndex >= 0 && logFile->m_linkIndex != logFile->m_currentIndex)
    {
        updateLinkNoLock(logFile);
    }
    logFile->closeEntity();
    LogManagerHelper::closeFileLock(logFile->m_linkFileLock);
    if (logFile->m_config.m_archiveOldLog)
    {
        archiveSessionNoLock(logFile);
    }
    m_logMap.erase(it);
    delete logFile;
}

void LogManager::writeLineNoLock(LogManagerFile* logFile, const std::string& line)
{
    if (logFile->m_rollRequested.load())
    {
        rollNoLock(logFile);
    }
    appendLine(logFile, line);
}

void LogManager::appendLine(LogManagerFile* logFile, const std::string& line)
{
    if (!logFile->append(line))
    {
        std::fprintf(stderr, "LogManager: durable write failed for %s\n", logFile->m_currentEntityPath.c_str());
        return;
    }
    if (logFile->m_config.m_outputConsole)
    {
        try
        {
            if (std::cout << line << std::endl)
            {
                return;
            }
        }
        catch (...)
        {
            // 宿主可以为标准输出开启异常或替换缓冲区。此时文件记录已提交。
        }
        std::fputs("LogManager: console output failed\n", stderr);
    }
}

void LogManager::rollNoLock(LogManagerFile* logFile)
{
    int32_t index = logFile->m_currentIndex;
    while (index < INT32_MAX)
    {
        ++index;
        const std::string path = LogManagerHelper::buildEntityPath(logFile->m_logDir, logFile->m_baseName, index);
        if (openLogFileNoLock(logFile, index, path))
        {
            updateLinkNoLock(logFile);
            deleteExpiredLogNoLock(logFile);
            return;
        }
        if (!LogManagerHelper::exist(path))
        {
            std::fprintf(stderr, "LogManager: cannot open durable log %s\n", path.c_str());
            return;
        }
    }
}

bool LogManager::openLogFileNoLock(LogManagerFile* logFile, int32_t fileIndex, const std::string& entityPath)
{
    if (!logFile->openEntity(entityPath))
    {
        return false;
    }
    logFile->m_currentIndex = fileIndex;
    logFile->m_currentEntityPath = entityPath;
    logFile->m_entityPaths.push_back(entityPath);
    return true;
}

void LogManager::updateLinkNoLock(LogManagerFile* logFile)
{
    LogManagerHelper::closeFileLock(logFile->m_linkFileLock);
    if (logFile->m_linkIndex >= 0 && !LogManagerHelper::deleteFile(logFile->m_linkPath))
    {
        // 外部读取者可能暂时禁止删除。保留旧卷号，后续滚动或关闭时仍能重试。
        return;
    }
    logFile->m_linkIndex = -1;
    const std::string target = logFile->m_currentEntityPath.substr(logFile->m_logDir.size());
    if (LogManagerHelper::createSymbolicLinkFile(logFile->m_linkPath, target))
    {
        logFile->m_linkIndex = logFile->m_currentIndex;
        logFile->m_linkFileLock = LogManagerHelper::lockLinkDelete(logFile->m_linkPath);
    }
}

bool LogManager::openSessionNoLock(LogManagerFile* logFile)
{
#ifdef __EMSCRIPTEN__
    const int32_t pid = 0;
#else
    const int32_t pid = static_cast<int32_t>(CSystem::currentProcessPid());
#endif
    const std::string timeName = LogManagerHelper::currentTimeName();
    const std::string base = LogManagerHelper::buildBaseName(m_processName, pid, timeName);
    const std::string archiveBase = logFile->m_logDir + m_processName + "_log/" + timeName + "_" + CStringManager::toStringInt32(pid);
    for (uint64_t sequence = 0; ; ++sequence)
    {
        const std::string suffix = sequence == 0 ? "" : "_" + std::to_string(sequence);
        const std::string candidate = base + suffix;
        std::string linkPath = LogManagerHelper::buildLinkPath(logFile->m_logDir, candidate);
        std::string archiveDir = archiveBase + suffix;
        const std::string entity = LogManagerHelper::buildEntityPath(logFile->m_logDir, candidate, 0);
        if (!LogManagerHelper::exist(linkPath) &&
            !LogManagerHelper::exist(entity) &&
            !LogManagerHelper::exist(archiveDir))
        {
            logFile->m_baseName = candidate;
            logFile->m_linkPath = linkPath;
            logFile->m_archiveDir = archiveDir;
            if (openLogFileNoLock(logFile, 0, entity))
            {
                return true;
            }
            if (!LogManagerHelper::exist(entity))
            {
                std::fprintf(stderr, "LogManager: cannot open durable log %s\n", entity.c_str());
                return false;
            }
        }
    }
}

void LogManager::archiveSessionNoLock(LogManagerFile* logFile)
{
    if (logFile->m_entityPaths.empty() || logFile->m_archiveDir.empty() ||
        !LogManagerHelper::createDirectoryRecursive(logFile->m_archiveDir))
    {
        return;
    }
    std::string destination = LogManagerHelper::normalizeLogDir(logFile->m_archiveDir);
    bool currentMoved = false;
    for (const std::string& path : logFile->m_entityPaths)
    {
        bool moved = CSystem::movePathIfAbsent(path, destination + path.substr(logFile->m_logDir.size()));
        if (path == logFile->m_currentEntityPath)
        {
            currentMoved = moved;
        }
    }
    if (currentMoved && logFile->m_linkIndex == logFile->m_currentIndex)
    {
        CSystem::movePathIfAbsent(logFile->m_linkPath, destination + logFile->m_linkPath.substr(logFile->m_logDir.size()));
    }
}

void LogManager::deleteExpiredLogNoLock(LogManagerFile* logFile)
{
    const int32_t limit = logFile->m_config.m_maxFileCount;
    while (limit != -1 && logFile->m_entityPaths.size() > static_cast<size_t>(limit))
    {
        // 第 0 卷保留，第二项始终是最早仍存在的滚动卷。
        const auto oldest = logFile->m_entityPaths.begin() + 1;
        if (!LogManagerHelper::deleteFile(*oldest))
        {
            break;
        }
        logFile->m_entityPaths.erase(oldest);
    }
}

std::string LogManager::formatLineText(LogLevel flag, const std::string& fileMacro, const std::string& funName, const std::string& exeName, const std::string& intDateTime, int32_t threadId, const std::string& message)
{
    const char* strFlag;
    switch (flag)
    {
    case LogManager::LOG_DEBUG:
    {
#ifdef _DEBUG
        strFlag = "  DEBUG";
        break;
#else
        return "";
#endif
    }
    case LogManager::LOG_INFO:
    {
        strFlag = "   INFO";
        break;
    }
    case LogManager::LOG_WARNING:
    {
        strFlag = "WARNING";
        break;
    }
    case LogManager::LOG_ERROR:
    {
        strFlag = "  ERROR";
        break;
    }
    case LogManager::LOG_FATAL:
    {
        strFlag = "  FATAL";
        break;
    }
    case LogManager::LOG_BEGIN:
    {
        strFlag = "  BEGIN";
        break;
    }
    case LogManager::LOG_END:
    {
        strFlag = "    END";
        break;
    }
    default:
    {
        return "";
    }
    }

#ifndef __EMSCRIPTEN__
    if (threadId == 0)
    {
        threadId = static_cast<int32_t>(CSystem::SystemThreadId());
    }
#endif
    const std::string thread = CStringManager::toStringInt64(static_cast<uint32_t>(threadId));
    std::string line = "[" + (intDateTime.empty() ? IntDateTime().timeToString() : intDateTime) + "][" +
        strFlag + "][ThreadId:" + thread + "][" + (exeName.empty() ? m_exeName : exeName) + "][" +
        LogManagerHelper::fileName(fileMacro) + "][" + funName + "]";
    if (flag != LOG_BEGIN && flag != LOG_END)
    {
        line += " : ";
        line += message;
    }
    return line;
}

// 使用示例。宿主提供可持久化的可写目录作为命令行参数。
// int main(int argc, char** argv)
// {
//     if (argc != 2)
//     {
//         return 2;
//     }
//     LogManager& log = LogManager::instance();
//     LogManagerConfig config;
//     config.m_path = argv[1];
//     config.m_archiveOldLog = false;
//     config.m_maxFileBytes = -1;
//     log.setDefaultConfig(config);
//     log.set(true, false);
//     log.init();
//     LOGINFO("application started");
//     config.m_fileId = 7;
//     log.init(config);
//     LOGWARNING_EX_EVERY(7, 5, "queue depth=%d", 3);
//     log.print(7, LogManager::LOG_ERROR, __FILE__, __FUNCTION__, "", "", 0, "result=%d", 12);
//     const bool defaultOpen = log.findFileId(config.m_path) == 0;
//     log.uninit(0);
//     const bool secondOpen = log.findFileId(config.m_path) == config.m_fileId;
//     log.deleteFile(config.m_fileId);
//     log.uninitAll();
//     return defaultOpen && secondOpen ? 0 : 1;
// }