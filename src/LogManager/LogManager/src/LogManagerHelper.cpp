#include "LogManagerHelper.h"
#include "LogManagerConfig.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include <sys/stat.h>

#ifdef _WIN32
#include <windows.h>
#elif defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
#include <cerrno>
#include <unistd.h>
#endif

std::string LogManagerHelper::normalizeLogDir(const std::string& path)
{
    std::string logDir = path;
#ifndef __EMSCRIPTEN__
    if (logDir.empty())
    {
        logDir = CSystem::GetCurrentExePath();
    }
#endif
    if (logDir.empty())
    {
        logDir = ".";
    }
#ifdef _WIN32
    CStringManager::Replace(logDir, "/", "\\");
    if (logDir.back() != '\\')
    {
        logDir += "\\";
    }
#else
    if (logDir.back() != '/')
    {
        logDir += "/";
    }
#endif
    return logDir;
}

bool LogManagerHelper::createDirectoryRecursive(const std::string& path)
{
#ifdef _WIN32
    return CSystem::createDirectories(path);
#else
    if (path.empty())
    {
        return false;
    }
    const size_t last = path.find_last_not_of('/');
    if (last == std::string::npos)
    {
        return true;
    }
    const std::string directory = path.substr(0, last + 1);
    struct stat info;
    if (stat(directory.c_str(), &info) == 0)
    {
        return S_ISDIR(info.st_mode);
    }
    const size_t slash = directory.rfind('/');
    if (slash != std::string::npos && slash > 0 && !createDirectoryRecursive(directory.substr(0, slash)))
    {
        return false;
    }
    if (mkdir(directory.c_str(), 0777) != 0)
    {
        return errno == EEXIST && stat(directory.c_str(), &info) == 0 && S_ISDIR(info.st_mode);
    }
#ifdef __EMSCRIPTEN__
    return true;
#else
    return CSystem::syncParentDirectory(directory);
#endif
#endif
}

std::string LogManagerHelper::currentTimeName()
{
    IntDateTime now;
    std::string time = now.timeToString();
    CStringManager::Replace(time, ":", "");
    return now.dateNumToString() + "_" + time;
}

std::string LogManagerHelper::buildBaseName(const std::string& processName, int32_t pid, const std::string& timeName)
{
    std::string result;
    CStringManager::Format(result, "%s_%d_%s", processName.c_str(), pid, timeName.c_str());
    return result;
}

std::string LogManagerHelper::buildLinkPath(const std::string& logDir, const std::string& baseName)
{
    return logDir + baseName + ".log";
}

std::string LogManagerHelper::buildEntityPath(const std::string& logDir, const std::string& baseName, int32_t fileIndex)
{
    std::string result;
    CStringManager::Format(result, "%s%s.%d.log", logDir.c_str(), baseName.c_str(), fileIndex);
    return result;
}

bool LogManagerHelper::exist(const std::string& path)
{
#ifdef _WIN32
    return GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES;
#elif defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
    struct stat fileStat;
    return lstat(path.c_str(), &fileStat) == 0;
#else
    return CSystem::DirOrFileExist(path);
#endif
}

bool LogManagerHelper::deleteFile(const std::string& path)
{
    if (path.empty())
    {
        return true;
    }
#ifdef __EMSCRIPTEN__
    return unlink(path.c_str()) == 0 || errno == ENOENT;
#else
    if (CSystem::deleteFile(path.c_str()))
    {
        return true;
    }
    // 只有明确不存在才视为已清理。权限和设备错误仍需保留卷的归属记录。
#ifdef _WIN32
    if (GetFileAttributesA(path.c_str()) != INVALID_FILE_ATTRIBUTES)
    {
        return false;
    }
    const DWORD error = GetLastError();
    return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
#else
    struct stat info;
    return lstat(path.c_str(), &info) != 0 && errno == ENOENT;
#endif
#endif
}

std::string LogManagerHelper::fileName(const std::string& path)
{
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos)
    {
        return path;
    }
    return path.substr(pos + 1);
}

bool LogManagerHelper::createSymbolicLinkFile(const std::string& linkPath, const std::string& targetPath)
{
#ifdef _WIN32
    enableSymbolicLinkPrivilege();
    return ::CreateSymbolicLinkA(linkPath.c_str(), targetPath.c_str(), 0) != FALSE;
#elif defined(__unix__) || defined(__APPLE__) || defined(__EMSCRIPTEN__)
    return symlink(targetPath.c_str(), linkPath.c_str()) == 0;
#else
    return false;
#endif
}

void* LogManagerHelper::lockLinkDelete(const std::string& path)
{
#ifdef _WIN32
    HANDLE file = ::CreateFileA(path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        nullptr,
        OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT,
        nullptr);
    if (file == INVALID_HANDLE_VALUE)
    {
        return nullptr;
    }
    return file;
#else
    (void)path;
    return nullptr;
#endif
}

void LogManagerHelper::closeFileLock(void*& lockHandle)
{
#ifdef _WIN32
    if (lockHandle != nullptr)
    {
        ::CloseHandle(static_cast<HANDLE>(lockHandle));
        lockHandle = nullptr;
    }
#else
    lockHandle = nullptr;
#endif
}

int64_t LogManagerHelper::normalizeMaxFileBytes(int64_t maxFileBytes)
{
    return maxFileBytes != -1 && maxFileBytes <= 0 ? LogManagerConfig().m_maxFileBytes : maxFileBytes;
}

int32_t LogManagerHelper::normalizeMaxFileCount(int32_t maxFileCount)
{
    return maxFileCount != -1 && maxFileCount < 2 ? 2 : maxFileCount;
}

int32_t LogManagerHelper::normalizeCheckFileSizeInterval(int32_t checkFileSizeInterval)
{
    return checkFileSizeInterval <= 0 ? 1 : checkFileSizeInterval;
}

#ifdef _WIN32
void LogManagerHelper::enableSymbolicLinkPrivilege()
{
    HANDLE token = nullptr;
    if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
    {
        return;
    }

    TOKEN_PRIVILEGES privileges;
    LUID luid;
    if (!::LookupPrivilegeValueA(nullptr, "SeCreateSymbolicLinkPrivilege", &luid))
    {
        ::CloseHandle(token);
        return;
    }

    privileges.PrivilegeCount = 1;
    privileges.Privileges[0].Luid = luid;
    privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    ::AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(privileges), nullptr, nullptr);
    ::CloseHandle(token);
}
#endif