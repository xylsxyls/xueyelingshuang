#include "LogManagerHelper.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include <algorithm>
#include <cstdlib>
#include <sys/stat.h>

#ifdef _WIN32
#include <io.h>
#include <windows.h>
#elif __unix__
#include <dirent.h>
#include <unistd.h>
#endif

int64_t LogManagerHelper::defaultMaxFileBytes()
{
	return 100LL * 1024LL * 1024LL;
}

std::string LogManagerHelper::normalizeLogDir(const std::string& path)
{
	std::string logDir = path.empty() ? CSystem::GetCurrentExePath() : path;
	if (logDir.empty())
	{
		logDir = ".";
	}
	char lastChar = logDir[logDir.size() - 1];
	if (lastChar != '/' && lastChar != '\\')
	{
#ifdef _WIN32
		logDir += "\\";
#else
		logDir += "/";
#endif
	}
	return logDir;
}

bool LogManagerHelper::createDirectoryRecursive(const std::string& path)
{
	if (path.empty() || CSystem::DirOrFileExist(path))
	{
		return true;
	}

	std::string trimPath = path;
	while (!trimPath.empty() && (trimPath[trimPath.size() - 1] == '/' || trimPath[trimPath.size() - 1] == '\\'))
	{
		trimPath.erase(trimPath.size() - 1);
	}
	if (trimPath.empty() || CSystem::DirOrFileExist(trimPath))
	{
		return true;
	}

	size_t pos = trimPath.find_last_of("/\\");
	if (pos != std::string::npos)
	{
#ifdef _WIN32
		if (pos > 2)
		{
			if (!createDirectoryRecursive(trimPath.substr(0, pos)))
			{
				return false;
			}
		}
#else
		if (pos > 0)
		{
			if (!createDirectoryRecursive(trimPath.substr(0, pos)))
			{
				return false;
			}
		}
#endif
	}

	return CSystem::CreateDir(trimPath) || CSystem::DirOrFileExist(trimPath);
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

int64_t LogManagerHelper::fileSize(const std::string& path)
{
#ifdef _WIN32
	struct _stat64 fileStat;
	if (_stat64(path.c_str(), &fileStat) != 0)
	{
		return 0;
	}
	return static_cast<int64_t>(fileStat.st_size);
#elif __unix__
	struct stat fileStat;
	if (stat(path.c_str(), &fileStat) != 0)
	{
		return 0;
	}
	return static_cast<int64_t>(fileStat.st_size);
#else
	return 0;
#endif
}

bool LogManagerHelper::exist(const std::string& path)
{
	return CSystem::DirOrFileExist(path);
}

bool LogManagerHelper::deleteFile(const std::string& path)
{
	if (path.empty())
	{
		return true;
	}
	return CSystem::deleteFile(path.c_str()) || !exist(path);
}

bool LogManagerHelper::renameFile(const std::string& oldPath, const std::string& newPath)
{
	if (oldPath.empty() || newPath.empty() || !exist(oldPath))
	{
		return false;
	}
	deleteFile(newPath);
	return CSystem::rename(oldPath, newPath);
}

std::vector<std::string> LogManagerHelper::listTopFiles(const std::string& logDir)
{
	std::vector<std::string> result;
	std::string realLogDir = normalizeLogDir(logDir);
#ifdef _WIN32
	std::string findPath = realLogDir + "*";
	struct _finddata_t fileInfo;
	intptr_t handle = _findfirst(findPath.c_str(), &fileInfo);
	if (handle == -1)
	{
		return result;
	}
	do
	{
		std::string name = fileInfo.name;
		if (name == "." || name == "..")
		{
			continue;
		}
		if ((fileInfo.attrib & _A_SUBDIR) == 0)
		{
			result.push_back(realLogDir + name);
		}
	}
	while (_findnext(handle, &fileInfo) == 0);
	_findclose(handle);
#elif __unix__
	DIR* dir = opendir(realLogDir.c_str());
	if (dir == nullptr)
	{
		return result;
	}
	struct dirent* item = nullptr;
	while ((item = readdir(dir)) != nullptr)
	{
		std::string name = item->d_name;
		if (name == "." || name == "..")
		{
			continue;
		}
		if (item->d_type != DT_DIR)
		{
			result.push_back(realLogDir + name);
		}
	}
	closedir(dir);
#endif
	return result;
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

bool LogManagerHelper::parseOldLogFile(const std::string& filePath, const std::string& processName, LogManagerOldFile& oldFile)
{
	std::string name = fileName(filePath);
	if (name.size() <= 4 || name.substr(name.size() - 4) != ".log")
	{
		return false;
	}

	std::string withoutSuffix = name.substr(0, name.size() - 4);
	int32_t fileIndex = -1;
	std::string baseName = withoutSuffix;
	size_t dotPos = withoutSuffix.find_last_of('.');
	if (dotPos != std::string::npos)
	{
		std::string indexText = withoutSuffix.substr(dotPos + 1);
		if (isNumber(indexText))
		{
			fileIndex = static_cast<int32_t>(std::atoi(indexText.c_str()));
			baseName = withoutSuffix.substr(0, dotPos);
		}
	}

	std::string prefix = processName + "_";
	if (baseName.find(prefix) != 0)
	{
		return false;
	}
	std::string remain = baseName.substr(prefix.size());
	std::vector<std::string> parts = CStringManager::split(remain, "_");
	if (parts.size() != 3 || !isNumber(parts[0]) || !isNumber(parts[1]) || !isNumber(parts[2]))
	{
		return false;
	}

	oldFile.m_filePath = filePath;
	oldFile.m_fileName = name;
	oldFile.m_baseName = baseName;
	oldFile.m_pid = static_cast<int32_t>(std::atoi(parts[0].c_str()));
	oldFile.m_date = parts[1];
	oldFile.m_time = parts[2];
	oldFile.m_fileIndex = fileIndex;
	return true;
}

bool LogManagerHelper::createSymbolicLinkFile(const std::string& linkPath, const std::string& targetPath)
{
	deleteFile(linkPath);
#ifdef _WIN32
	enableSymbolicLinkPrivilege();
	return ::CreateSymbolicLinkA(linkPath.c_str(), targetPath.c_str(), 0) != FALSE;
#elif __unix__
	return symlink(targetPath.c_str(), linkPath.c_str()) == 0;
#else
	return false;
#endif
}

void* LogManagerHelper::lockFileDelete(const std::string& path, bool openSymbolicLink)
{
#ifdef _WIN32
	DWORD flags = FILE_ATTRIBUTE_NORMAL;
	if (openSymbolicLink)
	{
		flags |= FILE_FLAG_OPEN_REPARSE_POINT;
	}
	HANDLE file = ::CreateFileA(path.c_str(),
		GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		nullptr,
		OPEN_EXISTING,
		flags,
		nullptr);
	if (file == INVALID_HANDLE_VALUE)
	{
		return nullptr;
	}
	return file;
#else
	(void)path;
	(void)openSymbolicLink;
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
	return maxFileBytes <= 0 ? defaultMaxFileBytes() : maxFileBytes;
}

int32_t LogManagerHelper::normalizeMaxFileCount(int32_t maxFileCount)
{
	return maxFileCount < 2 ? 2 : maxFileCount;
}

int32_t LogManagerHelper::normalizeCheckFileSizeInterval(int32_t checkFileSizeInterval)
{
	return checkFileSizeInterval <= 0 ? 1 : checkFileSizeInterval;
}

bool LogManagerHelper::enableSymbolicLinkPrivilege()
{
#ifdef _WIN32
	HANDLE token = nullptr;
	if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
	{
		return false;
	}

	TOKEN_PRIVILEGES privileges;
	LUID luid;
	if (!::LookupPrivilegeValueA(nullptr, "SeCreateSymbolicLinkPrivilege", &luid))
	{
		::CloseHandle(token);
		return false;
	}

	privileges.PrivilegeCount = 1;
	privileges.Privileges[0].Luid = luid;
	privileges.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	::AdjustTokenPrivileges(token, FALSE, &privileges, sizeof(privileges), nullptr, nullptr);
	bool ok = ::GetLastError() == ERROR_SUCCESS;
	::CloseHandle(token);
	return ok;
#else
	return true;
#endif
}

bool LogManagerHelper::isNumber(const std::string& value)
{
	if (value.empty())
	{
		return false;
	}
	for (size_t i = 0; i < value.size(); ++i)
	{
		if (value[i] < '0' || value[i] > '9')
		{
			return false;
		}
	}
	return true;
}