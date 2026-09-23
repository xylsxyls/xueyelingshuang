#include "LogManager.h"
#include "LogManagerFile.h"
#include "LogManagerHelper.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"
#include <fstream>
#include <iostream>
#include <new>
#include <sstream>
#include <stdarg.h>
#include <vector>

LogManager::LogManager() :
m_exeName(CSystem::GetCurrentExeFullName()),
m_processName(CSystem::GetCurrentExeName()),
m_logMap(),
m_usedLogNames(),
m_writeBeginEnd(true),
m_writeLog(true),
m_isProcessMutex(false),
m_writeMutex(nullptr)
{
	m_writeMutex = new (std::nothrow) ReadWriteMutex;
}

LogManager::~LogManager()
{
	uninitAll();
	if (m_writeMutex != nullptr)
	{
		delete m_writeMutex;
		m_writeMutex = nullptr;
	}
}

LogManager& LogManager::instance()
{
	static LogManager s_logManager;
	return s_logManager;
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

void LogManager::init(const LogManagerConfig& config)
{
	if (m_writeMutex == nullptr || !m_writeLog)
	{
		return;
	}

	WriteLock writeLock(*m_writeMutex);
	initNoLock(config);
}

void LogManager::set(bool writeLog, bool writeBeginEnd)
{
	m_writeLog = writeLog;
	m_writeBeginEnd = writeBeginEnd;
}

void LogManager::changeMutex(bool isProcessMutex)
{
	if (isProcessMutex == m_isProcessMutex)
	{
		return;
	}

	ReadWriteMutexBase* newMutex = nullptr;
	if (isProcessMutex)
	{
		newMutex = new (std::nothrow) ProcessReadWriteMutex("LogManager_Mutex");
	}
	else
	{
		newMutex = new (std::nothrow) ReadWriteMutex;
	}
	if (newMutex == nullptr)
	{
		return;
	}
	if (m_writeMutex != nullptr)
	{
		delete m_writeMutex;
	}
	m_isProcessMutex = isProcessMutex;
	m_writeMutex = newMutex;
}

void LogManager::print(int32_t fileId, LogLevel flag, const std::string& fileMacro, const std::string& funName, const std::string& exeName, const std::string& intDateTime, int32_t threadId, const char* format, ...)
{
	if (m_writeMutex == nullptr || !m_writeLog)
	{
		return;
	}

	va_list args;
	va_start(args, format);
	std::string line = formatLine(flag, fileMacro, funName, exeName, intDateTime, threadId, format, args);
	va_end(args);
	if (line.empty())
	{
		return;
	}

	WriteLock writeLock(*m_writeMutex);
	LogManagerFile* logFile = getLogFile(fileId);
	if (logFile == nullptr && fileId == 0)
	{
		LogManagerConfig config;
		logFile = initNoLock(config);
	}
	if (logFile == nullptr)
	{
		return;
	}

	writeLineNoLock(logFile, flag, line);
}

void LogManager::uninit(int32_t fileId)
{
	if (m_writeMutex == nullptr)
	{
		return;
	}

	WriteLock writeLock(*m_writeMutex);
	uninitNoLock(fileId, true);
}

void LogManager::uninitAll()
{
	if (m_writeMutex == nullptr)
	{
		return;
	}

	std::vector<int32_t> fileIds;
	{
		WriteLock writeLock(*m_writeMutex);
		for (std::map<int32_t, LogManagerFile*>::iterator it = m_logMap.begin(); it != m_logMap.end(); ++it)
		{
			fileIds.push_back(it->first);
		}
		for (size_t i = 0; i < fileIds.size(); ++i)
		{
			uninitNoLock(fileIds[i], true);
		}
	}
}

void LogManager::deleteFile(int32_t fileId)
{
	if (m_writeMutex == nullptr)
	{
		return;
	}

	std::vector<std::string> deletePaths;
	{
		WriteLock writeLock(*m_writeMutex);
		LogManagerFile* logFile = getLogFile(fileId);
		if (logFile == nullptr)
		{
			return;
		}
		if (logFile->m_linkCreated)
		{
			deletePaths.push_back(logFile->m_linkPath);
		}
		deletePaths.insert(deletePaths.end(), logFile->m_entityPaths.begin(), logFile->m_entityPaths.end());
		uninitNoLock(fileId, false);
	}

	for (size_t i = 0; i < deletePaths.size(); ++i)
	{
		LogManagerHelper::deleteFile(deletePaths[i]);
	}
}

int32_t LogManager::findFileId(const std::string& path)
{
	if (m_writeMutex == nullptr)
	{
		return -1;
	}

	WriteLock writeLock(*m_writeMutex);
	for (std::map<int32_t, LogManagerFile*>::iterator it = m_logMap.begin(); it != m_logMap.end(); ++it)
	{
		LogManagerFile* logFile = it->second;
		if (logFile != nullptr && (logFile->m_config.m_path == path || logFile->m_logDir == path || logFile->m_linkPath == path || logFile->m_currentEntityPath == path))
		{
			return it->first;
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

std::string LogManager::getLogPath(int32_t fileId)
{
	if (m_writeMutex == nullptr)
	{
		return "";
	}

	WriteLock writeLock(*m_writeMutex);
	LogManagerFile* logFile = getLogFile(fileId);
	if (logFile == nullptr)
	{
		return "";
	}
	return logFile->m_currentEntityPath;
}

LogManagerFile* LogManager::initNoLock(const LogManagerConfig& config)
{
	LogManagerFile* oldLogFile = getLogFile(config.m_fileId);
	if (oldLogFile != nullptr)
	{
		return oldLogFile;
	}

	LogManagerConfig realConfig = config;
	realConfig.m_maxFileBytes = LogManagerHelper::normalizeMaxFileBytes(realConfig.m_maxFileBytes);
	realConfig.m_maxFileCount = LogManagerHelper::normalizeMaxFileCount(realConfig.m_maxFileCount);
	realConfig.m_checkFileSizeInterval = LogManagerHelper::normalizeCheckFileSizeInterval(realConfig.m_checkFileSizeInterval);

	std::string logDir = LogManagerHelper::normalizeLogDir(realConfig.m_path);
	if (!LogManagerHelper::createDirectoryRecursive(logDir))
	{
		return nullptr;
	}

	LogManagerFile* logFile = new (std::nothrow) LogManagerFile;
	if (logFile == nullptr)
	{
		return nullptr;
	}
	logFile->m_config = realConfig;
	logFile->m_logDir = logDir;
	prepareSessionNoLock(logFile);

	if (!openLogFileNoLock(logFile, 0))
	{
		delete logFile;
		return nullptr;
	}
	updateLinkNoLock(logFile);
	m_logMap[realConfig.m_fileId] = logFile;

	if (m_writeBeginEnd)
	{
		std::string line = formatLineText(LOG_BEGIN, __FILE__, __FUNCTION__, "", "", 0, "");
		writeLineNoLock(logFile, LOG_BEGIN, line);
	}
	return logFile;
}

void LogManager::uninitNoLock(int32_t fileId, bool writeEnd)
{
	std::map<int32_t, LogManagerFile*>::iterator it = m_logMap.find(fileId);
	if (it == m_logMap.end())
	{
		return;
	}

	LogManagerFile* logFile = it->second;
	if (writeEnd && m_writeBeginEnd && logFile != nullptr)
	{
		std::string line = formatLineText(LOG_END, __FILE__, __FUNCTION__, "", "", 0, "");
		writeLineNoLock(logFile, LOG_END, line);
	}
	if (logFile != nullptr)
	{
		closeLogFileNoLock(logFile);
		LogManagerHelper::closeFileLock(logFile->m_linkFileLock);
		if (writeEnd && logFile->m_config.m_archiveOldLog)
		{
			archiveSessionNoLock(logFile);
		}
	}
	m_logMap.erase(it);
	delete logFile;
}

void LogManager::writeLineNoLock(LogManagerFile* logFile, LogLevel flag, const std::string& line)
{
	if (logFile == nullptr || logFile->m_logFile == nullptr || !logFile->m_logFile->is_open())
	{
		return;
	}

	checkRollNoLock(logFile);
	if (logFile->m_logFile == nullptr || !logFile->m_logFile->is_open())
	{
		return;
	}
	if (flag == LOG_BEGIN && LogManagerHelper::fileSize(logFile->m_currentEntityPath) > 0)
	{
		*logFile->m_logFile << std::endl;
		if (logFile->m_config.m_outputConsole)
		{
			std::cout << std::endl;
		}
	}
	*logFile->m_logFile << line << std::endl;
	if (logFile->m_config.m_outputConsole)
	{
		std::cout << line << std::endl;
	}
	++logFile->m_writeCountSinceSizeCheck;
}

void LogManager::checkRollNoLock(LogManagerFile* logFile)
{
	if (logFile == nullptr)
	{
		return;
	}
	if (logFile->m_writeCountSinceSizeCheck < logFile->m_config.m_checkFileSizeInterval)
	{
		return;
	}

	logFile->m_writeCountSinceSizeCheck = 0;
	// 使用已打开写流的实际位置，避免Windows目录元数据滞后导致一直不滚动。
	std::streampos position = logFile->m_logFile->tellp();
	if (position != std::streampos(-1) && static_cast<std::streamoff>(position) > logFile->m_config.m_maxFileBytes)
	{
		rollNoLock(logFile);
	}
}

bool LogManager::rollNoLock(LogManagerFile* logFile)
{
	if (logFile == nullptr)
	{
		return false;
	}

	int32_t nextIndex = logFile->m_currentIndex + 1;
	closeLogFileNoLock(logFile);
	if (!openLogFileNoLock(logFile, nextIndex))
	{
		openLogFileNoLock(logFile, nextIndex - 1);
		return false;
	}
	updateLinkNoLock(logFile);
	deleteExpiredLogNoLock(logFile);
	return true;
}

bool LogManager::openLogFileNoLock(LogManagerFile* logFile, int32_t fileIndex)
{
	if (logFile == nullptr)
	{
		return false;
	}

	std::string entityPath = LogManagerHelper::buildEntityPath(logFile->m_logDir, logFile->m_baseName, fileIndex);
	if (logFile->m_entityPaths.find(entityPath) == logFile->m_entityPaths.end() && LogManagerHelper::exist(entityPath))
	{
		return false;
	}
	logFile->m_logFile = new (std::nothrow) std::ofstream(entityPath.c_str(), std::ios::out | std::ios::app);
	if (logFile->m_logFile == nullptr || !logFile->m_logFile->is_open())
	{
		if (logFile->m_logFile != nullptr)
		{
			delete logFile->m_logFile;
			logFile->m_logFile = nullptr;
		}
		return false;
	}
	logFile->m_currentIndex = fileIndex;
	logFile->m_currentEntityPath = entityPath;
	logFile->m_entityPaths.insert(entityPath);
	logFile->m_currentFileLock = LogManagerHelper::lockFileDelete(logFile->m_currentEntityPath, false);
	logFile->m_writeCountSinceSizeCheck = 0;
	return true;
}

void LogManager::closeLogFileNoLock(LogManagerFile* logFile)
{
	if (logFile == nullptr)
	{
		return;
	}
	if (logFile->m_logFile != nullptr)
	{
		logFile->m_logFile->flush();
		logFile->m_logFile->close();
		delete logFile->m_logFile;
		logFile->m_logFile = nullptr;
	}
	LogManagerHelper::closeFileLock(logFile->m_currentFileLock);
}

void LogManager::updateLinkNoLock(LogManagerFile* logFile)
{
	if (logFile == nullptr)
	{
		return;
	}
	LogManagerHelper::closeFileLock(logFile->m_linkFileLock);
	logFile->m_linkCreated = LogManagerHelper::createSymbolicLinkFile(logFile->m_linkPath, LogManagerHelper::fileName(logFile->m_currentEntityPath));
	if (logFile->m_linkCreated)
	{
		logFile->m_linkFileLock = LogManagerHelper::lockFileDelete(logFile->m_linkPath, true);
	}
}

void LogManager::prepareSessionNoLock(LogManagerFile* logFile)
{
	std::string base = LogManagerHelper::buildBaseName(m_processName, static_cast<int32_t>(CSystem::currentProcessPid()), LogManagerHelper::currentTimeName());
	for (uint64_t sequence = 0; ; ++sequence)
	{
		std::ostringstream name;
		name << base;
		if (sequence != 0)
		{
			name << "_" << sequence;
		}
		std::string candidate = name.str();
		std::string linkPath = LogManagerHelper::buildLinkPath(logFile->m_logDir, candidate);
		std::string archiveDir = LogManagerHelper::buildArchiveDir(logFile->m_logDir, m_processName, candidate);
		if (m_usedLogNames.find(candidate) == m_usedLogNames.end() &&
			!LogManagerHelper::exist(linkPath) &&
			!LogManagerHelper::exist(LogManagerHelper::buildEntityPath(logFile->m_logDir, candidate, 0)) &&
			!LogManagerHelper::exist(archiveDir))
		{
			m_usedLogNames[candidate] = true;
			logFile->m_baseName = candidate;
			logFile->m_linkPath = linkPath;
			logFile->m_archiveDir = archiveDir;
			return;
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
	for (std::set<std::string>::const_iterator it = logFile->m_entityPaths.begin(); it != logFile->m_entityPaths.end(); ++it)
	{
		bool moved = LogManagerHelper::renameFile(*it, destination + LogManagerHelper::fileName(*it));
		if (*it == logFile->m_currentEntityPath)
		{
			currentMoved = moved;
		}
	}
	if (logFile->m_linkCreated && currentMoved)
	{
		LogManagerHelper::renameFile(logFile->m_linkPath, destination + LogManagerHelper::fileName(logFile->m_linkPath));
	}
}

void LogManager::deleteExpiredLogNoLock(LogManagerFile* logFile)
{
	if (logFile == nullptr)
	{
		return;
	}
	int32_t keepRollingCount = logFile->m_config.m_maxFileCount - 2;
	int32_t firstKeepIndex = logFile->m_currentIndex - keepRollingCount;
	for (int32_t fileIndex = 1; fileIndex < firstKeepIndex; ++fileIndex)
	{
		std::string path = LogManagerHelper::buildEntityPath(logFile->m_logDir, logFile->m_baseName, fileIndex);
		if (logFile->m_entityPaths.find(path) != logFile->m_entityPaths.end() && LogManagerHelper::deleteFile(path))
		{
			logFile->m_entityPaths.erase(path);
		}
	}
}

std::string LogManager::formatLine(LogLevel flag, const std::string& fileMacro, const std::string& funName, const std::string& exeName, const std::string& intDateTime, int32_t threadId, const char* format, va_list args)
{
	std::string message;
	if (format != nullptr)
	{
#ifdef _WIN32
		va_list argsCopy = args;
		int32_t size = _vscprintf(format, argsCopy);
#elif defined(__unix__) || defined(__APPLE__)
		va_list argsCopy;
		va_copy(argsCopy, args);
		int32_t size = vsnprintf(nullptr, 0, format, argsCopy);
		va_end(argsCopy);
#else
		int32_t size = 0;
#endif
		if (size > 0)
		{
			std::vector<char> buffer(static_cast<size_t>(size) + 1);
#ifdef _WIN32
			vsprintf_s(&buffer[0], buffer.size(), format, args);
#elif defined(__unix__) || defined(__APPLE__)
			vsnprintf(&buffer[0], buffer.size(), format, args);
#endif
			message.assign(&buffer[0], static_cast<size_t>(size));
		}
	}
	return formatLineText(flag, fileMacro, funName, exeName, intDateTime, threadId, message);
}

std::string LogManager::formatLineText(LogLevel flag, const std::string& fileMacro, const std::string& funName, const std::string& exeName, const std::string& intDateTime, int32_t threadId, const std::string& message)
{
	std::string strFlag;
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

	std::string fileMacroTemp;
	size_t right = fileMacro.find_last_of("/\\");
	if (right == std::string::npos)
	{
		fileMacroTemp = fileMacro;
	}
	else
	{
		fileMacroTemp = CStringManager::Mid(fileMacro, right + 1, fileMacro.length() - right - 1);
	}

	std::ostringstream oss;
	oss << "[" << (intDateTime.empty() ? IntDateTime().timeToString() : intDateTime) << "]";
	oss << "[" << strFlag << "]";
	oss << "[ThreadId:" << (threadId == 0 ? CSystem::SystemThreadId() : static_cast<uint32_t>(threadId)) << "]";
	oss << "[" << (exeName.empty() ? m_exeName : exeName) << "]";
	oss << "[" << fileMacroTemp << "]";
	oss << "[" << funName.c_str() << "]";
	if (flag != LOG_BEGIN && flag != LOG_END)
	{
		oss << " : " << message;
	}
	return oss.str();
}

// 自测入口：临时取消下面整个区块的行注释，将本工程临时编译为控制台程序；测完恢复注释。
// 全部用例与子进程分支都在同一个main内，无独立Test工程；Windows软链接用例需要对应权限。
//#include "CSystem/CSystemAPI.h"
//#include "CStringManager/CStringManagerAPI.h"
//#include <atomic>
//#include <chrono>
//#include <stdexcept>
//#include <cstdlib>
//#include <fstream>
//#include <functional>
//#include <iostream>
//#include <sstream>
//#include <thread>
//
//#ifdef _WIN32
//#include <process.h>
//#include <windows.h>
//#elif defined(__unix__) || defined(__APPLE__)
//#include <dirent.h>
//#include <sys/stat.h>
//#endif
//
//int main(int32_t argc, char** argv)
//{
//	// 同一个main的子进程分支，供父进程检查析构和多实例隔离。
//	if (argc == 6 && std::string(argv[1]) == "--archive-child")
//	{
//		const std::string mode = argv[3];
//		LogManagerConfig config;
//		config.m_fileId = 17;
//		config.m_path = argv[2];
//		config.m_maxFileBytes = 700;
//		config.m_maxFileCount = 3;
//		config.m_checkFileSizeInterval = 1;
//		config.m_archiveOldLog = mode != "noarchive";
//		LogManager::instance().init(config);
//		if (mode == "multi")
//		{
//			config.m_fileId = 18;
//			LogManager::instance().init(config);
//		}
//		const std::string payload(240, 'x');
//		for (int i = 0; i < 30; ++i)
//		{
//			LOGINFO_EX(17, "OWNER17 row=%d %s", i, payload.c_str());
//			if (mode == "multi") { LOGINFO_EX(18, "OWNER18 row=%d %s", i, payload.c_str()); }
//		}
//		std::ofstream(argv[4]) << "ready";
//		const std::chrono::steady_clock::time_point deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
//		while (!std::ifstream(argv[5]).good())
//		{
//			if (std::chrono::steady_clock::now() >= deadline) { return 3; }
//			std::this_thread::sleep_for(std::chrono::milliseconds(20));
//		}
//		LOGINFO_EX(17, "LAST17 before closing");
//		if (mode == "destructor") { return 0; }
//		if (mode == "delete")
//		{
//			LogManager::instance().deleteFile(17);
//			return 0;
//		}
//		LogManager::instance().uninit(17);
//		if (mode == "reinit")
//		{
//			LogManager::instance().init(config);
//			LOGINFO_EX(17, "SECOND_SESSION");
//			LogManager::instance().uninit(17);
//		}
//		if (mode == "multi") { LOGINFO_EX(18, "LAST18 still writable after closing 17"); }
//		LogManager::instance().uninit(17);
//		LogManager::instance().uninitAll();
//		LogManager::instance().uninitAll();
//		return 0;
//	}
//
//	int32_t totalCount = 0;
//	int32_t failCount = 0;
//	int32_t skipCount = 0;
//	std::function<void(bool, const std::string&)> check = [&totalCount, &failCount](bool ok, const std::string& name) -> void
//	{
//		++totalCount;
//		std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << std::endl;
//		if (!ok)
//		{
//			++failCount;
//		}
//	};
//	std::function<void(const std::string&)> skip = [&skipCount](const std::string& name) -> void
//	{
//		++skipCount;
//		std::cout << "[SKIP] " << name << std::endl;
//	};
//	std::function<std::string()> testRoot = []() -> std::string
//	{
//		std::string root = CSystem::GetCurrentExePath() + "LogManagerTest_" + CStringManager::toStringInt32(static_cast<int32_t>(CSystem::currentProcessPid())) + "/";
//		LogManagerHelper::createDirectoryRecursive(root);
//		return root;
//	};
//
//	// 仅测试使用的局部辅助逻辑：读取实际目录与文件，不访问日志管理器私有状态。
//	auto directories = [](const std::string& path) -> std::vector<std::string>
//	{
//		std::vector<std::string> result;
//		std::string directory = LogManagerHelper::normalizeLogDir(path);
//#ifdef _WIN32
//		WIN32_FIND_DATAA data;
//		HANDLE find = FindFirstFileA((directory + "*").c_str(), &data);
//		if (find == INVALID_HANDLE_VALUE) { return result; }
//		do
//		{
//			std::string name = data.cFileName;
//			if (name != "." && name != ".." && (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
//				!(data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) { result.push_back(directory + name); }
//		} while (FindNextFileA(find, &data));
//		FindClose(find);
//#elif defined(__unix__) || defined(__APPLE__)
//		DIR* dir = opendir(directory.c_str());
//		if (dir == nullptr) { return result; }
//		struct dirent* item = nullptr;
//		while ((item = readdir(dir)) != nullptr)
//		{
//			std::string name = item->d_name;
//			struct stat status;
//			if (name != "." && name != ".." && lstat((directory + name).c_str(), &status) == 0 &&
//				S_ISDIR(status.st_mode)) { result.push_back(directory + name); }
//		}
//		closedir(dir);
//#endif
//		return result;
//	};
//	std::function<std::vector<std::string>(const std::string&)> allFiles;
//	allFiles = [&allFiles, &directories](const std::string& path) -> std::vector<std::string>
//	{
//		std::vector<std::string> result = LogManagerHelper::listTopFiles(path);
//		std::vector<std::string> dirs = directories(path);
//		for (size_t i = 0; i < dirs.size(); ++i)
//		{
//			std::vector<std::string> nested = allFiles(dirs[i]);
//			result.insert(result.end(), nested.begin(), nested.end());
//		}
//		return result;
//	};
//	auto readFile = [](const std::string& path) -> std::string
//	{
//#ifdef _WIN32
//		HANDLE file = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
//			nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
//		if (file == INVALID_HANDLE_VALUE) { throw std::runtime_error("cannot read " + path); }
//		std::string text;
//		char bytes[4096];
//		DWORD count = 0;
//		BOOL ok = TRUE;
//		while ((ok = ReadFile(file, bytes, sizeof(bytes), &count, nullptr)) && count != 0) { text.append(bytes, count); }
//		CloseHandle(file);
//		if (!ok) { throw std::runtime_error("read failed " + path); }
//		return text;
//#else
//		std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);
//		if (!file) { throw std::runtime_error("cannot read " + path); }
//		std::ostringstream text;
//		text << file.rdbuf();
//		return text.str();
//#endif
//	};
//
//	std::function<int32_t(const std::string&)> countTopFiles = [](const std::string& path) -> int32_t
//	{
//		return static_cast<int32_t>(LogManagerHelper::listTopFiles(path).size());
//	};
//	std::function<std::string(const std::string&)> readLogText = [&allFiles](const std::string& path) -> std::string
//	{
//		std::ostringstream oss;
//		std::vector<std::string> files = allFiles(path);
//		for (size_t i = 0; i < files.size(); ++i)
//		{
//			LogManagerOldFile oldFile;
//			if (!LogManagerHelper::parseOldLogFile(files[i], CSystem::GetCurrentExeName(), oldFile) || oldFile.m_fileIndex < 0)
//			{
//				continue;
//			}
//			std::ifstream input(files[i].c_str(), std::ios::in | std::ios::binary);
//			oss << input.rdbuf();
//		}
//		return oss.str();
//	};
//	std::function<int32_t(const std::string&, const std::string&)> countToken = [](const std::string& text, const std::string& token) -> int32_t
//	{
//		int32_t count = 0;
//		size_t pos = 0;
//		while ((pos = text.find(token, pos)) != std::string::npos)
//		{
//			++count;
//			pos += token.size();
//		}
//		return count;
//	};
//	if (argc >= 3 && std::string(argv[1]) == "--child")
//	{
//		LogManagerConfig childConfig;
//		childConfig.m_fileId = 0;
//		childConfig.m_path = argv[2];
//		childConfig.m_maxFileBytes = 1024;
//		childConfig.m_maxFileCount = 5;
//		childConfig.m_checkFileSizeInterval = 1;
//		childConfig.m_archiveOldLog = true;
//		LogManager::instance().init(childConfig);
//		for (int32_t i = 0; i < 300; ++i)
//		{
//			LOGINFO("child process macro log %d %s", i, "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
//		}
//		LogManager::instance().uninitAll();
//		return 0;
//	}
//
//	std::string root = testRoot();
//	std::string linkCheckTarget = root + "link_check_target.log";
//	std::string linkCheckPath = root + "link_check.log";
//	{
//		std::ofstream targetFile(linkCheckTarget.c_str(), std::ios::out | std::ios::app);
//		targetFile << "target" << std::endl;
//	}
//	bool canCreateSymbolicLink = LogManagerHelper::createSymbolicLinkFile(linkCheckPath, linkCheckTarget);
//	LogManagerHelper::deleteFile(linkCheckPath);
//	LogManagerHelper::deleteFile(linkCheckTarget);
//
//	std::string macroDefaultRoot = root + "macro_default/";
//	LogManagerConfig macroDefaultConfig;
//	macroDefaultConfig.m_fileId = 0;
//	macroDefaultConfig.m_path = macroDefaultRoot;
//	macroDefaultConfig.m_maxFileBytes = 1024 * 1024;
//	macroDefaultConfig.m_checkFileSizeInterval = 1;
//	LogManager::instance().init(macroDefaultConfig);
//	LOGDEBUG("macro_default_debug");
//	LOGINFO("macro_default_info");
//	LOGWARNING("macro_default_warning");
//	LOGERROR("macro_default_error");
//	LOGFATAL("macro_default_fatal");
//	LogManager::instance().uninit(0);
//	std::string macroDefaultText = readLogText(macroDefaultRoot);
//#ifdef _DEBUG
//	check(countToken(macroDefaultText, "macro_default_debug") == 1, "LogManager LOGDEBUG macro output");
//#else
//	check(countToken(macroDefaultText, "macro_default_debug") == 0, "LogManager LOGDEBUG macro release filter");
//#endif
//	check(countToken(macroDefaultText, "macro_default_info") == 1, "LogManager LOGINFO macro output");
//	check(countToken(macroDefaultText, "macro_default_warning") == 1, "LogManager LOGWARNING macro output");
//	check(countToken(macroDefaultText, "macro_default_error") == 1, "LogManager LOGERROR macro output");
//	check(countToken(macroDefaultText, "macro_default_fatal") == 1, "LogManager LOGFATAL macro output");
//
//	std::string macroExRoot = root + "macro_ex/";
//	LogManagerConfig macroExConfig;
//	macroExConfig.m_fileId = 1001;
//	macroExConfig.m_path = macroExRoot;
//	macroExConfig.m_maxFileBytes = 1024 * 1024;
//	macroExConfig.m_checkFileSizeInterval = 1;
//	LogManager::instance().init(macroExConfig);
//	LOGDEBUG_EX(1001, "macro_ex_debug");
//	LOGINFO_EX(1001, "macro_ex_info");
//	LOGWARNING_EX(1001, "macro_ex_warning");
//	LOGERROR_EX(1001, "macro_ex_error");
//	LOGFATAL_EX(1001, "macro_ex_fatal");
//	LogManager::instance().uninit(1001);
//	std::string macroExText = readLogText(macroExRoot);
//#ifdef _DEBUG
//	check(countToken(macroExText, "macro_ex_debug") == 1, "LogManager LOGDEBUG_EX macro output");
//#else
//	check(countToken(macroExText, "macro_ex_debug") == 0, "LogManager LOGDEBUG_EX macro release filter");
//#endif
//	check(countToken(macroExText, "macro_ex_info") == 1, "LogManager LOGINFO_EX macro output");
//	check(countToken(macroExText, "macro_ex_warning") == 1, "LogManager LOGWARNING_EX macro output");
//	check(countToken(macroExText, "macro_ex_error") == 1, "LogManager LOGERROR_EX macro output");
//	check(countToken(macroExText, "macro_ex_fatal") == 1, "LogManager LOGFATAL_EX macro output");
//
//	std::string macroEveryRoot = root + "macro_every/";
//	LogManagerConfig macroEveryConfig;
//	macroEveryConfig.m_fileId = 0;
//	macroEveryConfig.m_path = macroEveryRoot;
//	macroEveryConfig.m_maxFileBytes = 1024 * 1024;
//	macroEveryConfig.m_checkFileSizeInterval = 1;
//	LogManager::instance().init(macroEveryConfig);
//	for (int32_t i = 0; i < 7; ++i)
//	{
//		LOGDEBUG_EVERY(3, "macro_every_debug %d", i);
//		LOGINFO_EVERY(3, "macro_every_info %d", i);
//		LOGWARNING_EVERY(3, "macro_every_warning %d", i);
//		LOGERROR_EVERY(3, "macro_every_error %d", i);
//		LOGFATAL_EVERY(3, "macro_every_fatal %d", i);
//	}
//	LogManager::instance().uninit(0);
//	std::string macroEveryText = readLogText(macroEveryRoot);
//#ifdef _DEBUG
//	check(countToken(macroEveryText, "macro_every_debug") == 3, "LogManager LOGDEBUG_EVERY macro output");
//#else
//	check(countToken(macroEveryText, "macro_every_debug") == 0, "LogManager LOGDEBUG_EVERY macro release filter");
//#endif
//	check(countToken(macroEveryText, "macro_every_info") == 3, "LogManager LOGINFO_EVERY macro output");
//	check(countToken(macroEveryText, "macro_every_warning") == 3, "LogManager LOGWARNING_EVERY macro output");
//	check(countToken(macroEveryText, "macro_every_error") == 3, "LogManager LOGERROR_EVERY macro output");
//	check(countToken(macroEveryText, "macro_every_fatal") == 3, "LogManager LOGFATAL_EVERY macro output");
//
//	std::string macroExEveryRoot = root + "macro_ex_every/";
//	LogManagerConfig macroExEveryConfig;
//	macroExEveryConfig.m_fileId = 1002;
//	macroExEveryConfig.m_path = macroExEveryRoot;
//	macroExEveryConfig.m_maxFileBytes = 1024 * 1024;
//	macroExEveryConfig.m_checkFileSizeInterval = 1;
//	LogManager::instance().init(macroExEveryConfig);
//	for (int32_t i = 0; i < 7; ++i)
//	{
//		LOGDEBUG_EX_EVERY(1002, 3, "macro_ex_every_debug %d", i);
//		LOGINFO_EX_EVERY(1002, 3, "macro_ex_every_info %d", i);
//		LOGWARNING_EX_EVERY(1002, 3, "macro_ex_every_warning %d", i);
//		LOGERROR_EX_EVERY(1002, 3, "macro_ex_every_error %d", i);
//		LOGFATAL_EX_EVERY(1002, 3, "macro_ex_every_fatal %d", i);
//	}
//	LogManager::instance().uninit(1002);
//	std::string macroExEveryText = readLogText(macroExEveryRoot);
//#ifdef _DEBUG
//	check(countToken(macroExEveryText, "macro_ex_every_debug") == 3, "LogManager LOGDEBUG_EX_EVERY macro output");
//#else
//	check(countToken(macroExEveryText, "macro_ex_every_debug") == 0, "LogManager LOGDEBUG_EX_EVERY macro release filter");
//#endif
//	check(countToken(macroExEveryText, "macro_ex_every_info") == 3, "LogManager LOGINFO_EX_EVERY macro output");
//	check(countToken(macroExEveryText, "macro_ex_every_warning") == 3, "LogManager LOGWARNING_EX_EVERY macro output");
//	check(countToken(macroExEveryText, "macro_ex_every_error") == 3, "LogManager LOGERROR_EX_EVERY macro output");
//	check(countToken(macroExEveryText, "macro_ex_every_fatal") == 3, "LogManager LOGFATAL_EX_EVERY macro output");
//
//	std::string macroOnceRoot = root + "macro_once/";
//	LogManagerConfig macroOnceConfig;
//	macroOnceConfig.m_fileId = 0;
//	macroOnceConfig.m_path = macroOnceRoot;
//	macroOnceConfig.m_maxFileBytes = 1024 * 1024;
//	macroOnceConfig.m_checkFileSizeInterval = 1;
//	LogManager::instance().init(macroOnceConfig);
//	for (int32_t i = 0; i < 4; ++i)
//	{
//		LOGDEBUG_ONCE("macro_once_debug %d", i);
//		LOGINFO_ONCE("macro_once_info %d", i);
//		LOGWARNING_ONCE("macro_once_warning %d", i);
//		LOGERROR_ONCE("macro_once_error %d", i);
//		LOGFATAL_ONCE("macro_once_fatal %d", i);
//	}
//	LogManager::instance().uninit(0);
//	std::string macroOnceText = readLogText(macroOnceRoot);
//#ifdef _DEBUG
//	check(countToken(macroOnceText, "macro_once_debug") == 1, "LogManager LOGDEBUG_ONCE macro output");
//#else
//	check(countToken(macroOnceText, "macro_once_debug") == 0, "LogManager LOGDEBUG_ONCE macro release filter");
//#endif
//	check(countToken(macroOnceText, "macro_once_info") == 1, "LogManager LOGINFO_ONCE macro output");
//	check(countToken(macroOnceText, "macro_once_warning") == 1, "LogManager LOGWARNING_ONCE macro output");
//	check(countToken(macroOnceText, "macro_once_error") == 1, "LogManager LOGERROR_ONCE macro output");
//	check(countToken(macroOnceText, "macro_once_fatal") == 1, "LogManager LOGFATAL_ONCE macro output");
//
//	std::string macroExOnceRoot = root + "macro_ex_once/";
//	LogManagerConfig macroExOnceConfig;
//	macroExOnceConfig.m_fileId = 1003;
//	macroExOnceConfig.m_path = macroExOnceRoot;
//	macroExOnceConfig.m_maxFileBytes = 1024 * 1024;
//	macroExOnceConfig.m_checkFileSizeInterval = 1;
//	LogManager::instance().init(macroExOnceConfig);
//	for (int32_t i = 0; i < 4; ++i)
//	{
//		LOGDEBUG_EX_ONCE(1003, "macro_ex_once_debug %d", i);
//		LOGINFO_EX_ONCE(1003, "macro_ex_once_info %d", i);
//		LOGWARNING_EX_ONCE(1003, "macro_ex_once_warning %d", i);
//		LOGERROR_EX_ONCE(1003, "macro_ex_once_error %d", i);
//		LOGFATAL_EX_ONCE(1003, "macro_ex_once_fatal %d", i);
//	}
//	LogManager::instance().uninit(1003);
//	std::string macroExOnceText = readLogText(macroExOnceRoot);
//#ifdef _DEBUG
//	check(countToken(macroExOnceText, "macro_ex_once_debug") == 1, "LogManager LOGDEBUG_EX_ONCE macro output");
//#else
//	check(countToken(macroExOnceText, "macro_ex_once_debug") == 0, "LogManager LOGDEBUG_EX_ONCE macro release filter");
//#endif
//	check(countToken(macroExOnceText, "macro_ex_once_info") == 1, "LogManager LOGINFO_EX_ONCE macro output");
//	check(countToken(macroExOnceText, "macro_ex_once_warning") == 1, "LogManager LOGWARNING_EX_ONCE macro output");
//	check(countToken(macroExOnceText, "macro_ex_once_error") == 1, "LogManager LOGERROR_EX_ONCE macro output");
//	check(countToken(macroExOnceText, "macro_ex_once_fatal") == 1, "LogManager LOGFATAL_EX_ONCE macro output");
//
//	LogManagerConfig config;
//	config.m_fileId = 1004;
//	config.m_path = root;
//	config.m_maxFileBytes = 1024;
//	config.m_maxFileCount = 5;
//	config.m_checkFileSizeInterval = 1;
//	config.m_outputConsole = false;
//	config.m_archiveOldLog = true;
//	LogManager::instance().init(config);
//	check(LogManager::instance().findFileId(root) == 1004, "LogManager init and findFileId");
//	for (int32_t i = 0; i < 500; ++i)
//	{
//		LOGINFO_EX(1004, "single process rolling macro log %d %s", i, "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
//	}
//	std::vector<std::string> files = LogManagerHelper::listTopFiles(root);
//	int32_t entityCount = 0;
//	bool hasZero = false;
//	bool hasLink = false;
//	int32_t maxIndex = -1;
//	std::string maxIndexPath;
//	std::string zeroPath;
//	std::string linkPath;
//	for (size_t i = 0; i < files.size(); ++i)
//	{
//		std::string name = LogManagerHelper::fileName(files[i]);
//		LogManagerOldFile oldFile;
//		if (LogManagerHelper::parseOldLogFile(files[i], CSystem::GetCurrentExeName(), oldFile) && oldFile.m_fileIndex >= 0)
//		{
//			++entityCount;
//			if (oldFile.m_fileIndex == 0)
//			{
//				hasZero = true;
//				zeroPath = files[i];
//			}
//			if (oldFile.m_fileIndex > maxIndex)
//			{
//				maxIndex = oldFile.m_fileIndex;
//				maxIndexPath = files[i];
//			}
//		}
//		else if (LogManagerHelper::parseOldLogFile(files[i], CSystem::GetCurrentExeName(), oldFile) && oldFile.m_fileIndex < 0)
//		{
//			hasLink = true;
//			linkPath = files[i];
//		}
//	}
//	check(hasZero, "LogManager keep zero file");
//	if (canCreateSymbolicLink)
//	{
//		check(hasLink, "LogManager create link");
//	}
//	else
//	{
//		skip("LogManager create link, current process has no symbolic link privilege");
//	}
//	check(entityCount <= 5, "LogManager rolling delete limit");
//	if (!maxIndexPath.empty())
//	{
//		check(!LogManagerHelper::deleteFile(maxIndexPath), "LogManager lock current entity file");
//	}
//	if (canCreateSymbolicLink && !linkPath.empty())
//	{
//		check(!LogManagerHelper::deleteFile(linkPath), "LogManager lock link file");
//	}
//	else if (!canCreateSymbolicLink)
//	{
//		skip("LogManager lock link file, current process has no symbolic link privilege");
//	}
//	if (!zeroPath.empty() && maxIndex > 0)
//	{
//		check(LogManagerHelper::deleteFile(zeroPath), "LogManager old entity can delete outside");
//	}
//	LogManager::instance().uninit(1004);
//
//	std::string archiveRoot = root + "archive_old/";
//	LogManagerHelper::createDirectoryRecursive(archiveRoot);
//	std::string archiveOldName = CSystem::GetCurrentExeName() + "_456_20260809_034122.0.log";
//	{
//		std::ofstream oldFile((archiveRoot + archiveOldName).c_str(), std::ios::out | std::ios::app);
//		oldFile << "old" << std::endl;
//	}
//	LogManagerConfig archiveConfig;
//	archiveConfig.m_fileId = 1005;
//	archiveConfig.m_path = archiveRoot;
//	archiveConfig.m_maxFileBytes = 2048;
//	archiveConfig.m_checkFileSizeInterval = 1;
//	archiveConfig.m_archiveOldLog = true;
//	LogManager::instance().init(archiveConfig);
//	LogManager::instance().uninit(1005);
//	check(LogManagerHelper::exist(archiveRoot + archiveOldName), "LogManager leaves old log in original directory");
//
//	std::string deleteRoot = root + "delete_old/";
//	LogManagerHelper::createDirectoryRecursive(deleteRoot);
//	std::string oldName = CSystem::GetCurrentExeName() + "_123_20260809_034122.0.log";
//	{
//		std::ofstream oldFile((deleteRoot + oldName).c_str(), std::ios::out | std::ios::app);
//		oldFile << "old" << std::endl;
//	}
//	LogManagerConfig deleteConfig;
//	deleteConfig.m_fileId = 1006;
//	deleteConfig.m_path = deleteRoot;
//	deleteConfig.m_archiveOldLog = false;
//	LogManager::instance().init(deleteConfig);
//	LogManager::instance().uninit(1006);
//	check(LogManagerHelper::exist(deleteRoot + oldName), "LogManager disabled archive also preserves old logs");
//
//	check(LogManager::needPrintEvery(0, -1), "LogManager every first");
//	check(!LogManager::needPrintEvery(1, -1), "LogManager once only");
//	check(LogManager::needPrintEvery(1, 0), "LogManager every zero interval");
//	check(LogManager::needPrintEvery(1000, 1000), "LogManager interval write");
//	check(!LogManager::needPrintEvery(999, 1000), "LogManager interval skip");
//
//	LogManagerConfig threadConfig;
//	threadConfig.m_fileId = 1007;
//	threadConfig.m_path = root + "thread/";
//	threadConfig.m_maxFileBytes = 10LL * 1024LL * 1024LL;
//	threadConfig.m_maxFileCount = 5;
//	threadConfig.m_checkFileSizeInterval = 100;
//	LogManager::instance().init(threadConfig);
//	std::atomic<int32_t> threadFailCount(0);
//	std::vector<std::thread> threads;
//	for (int32_t threadIndex = 0; threadIndex < 8; ++threadIndex)
//	{
//		threads.push_back(std::thread([threadIndex, &threadFailCount]() -> void
//		{
//			for (int32_t loopIndex = 0; loopIndex < 500; ++loopIndex)
//			{
//				LOGINFO_EX(1007, "thread macro log thread=%d loop=%d", threadIndex, loopIndex);
//			}
//		}));
//	}
//	for (size_t i = 0; i < threads.size(); ++i)
//	{
//		threads[i].join();
//	}
//	LogManager::instance().uninit(1007);
//	check(threadFailCount.load() == 0, "LogManager multithread macro pressure");
//	std::string threadText = readLogText(threadConfig.m_path);
//	check(countToken(threadText, "thread macro log") == 8 * 500, "LogManager multithread macro line count");
//
//	std::string deleteFileRoot = root + "delete_file/";
//	LogManagerConfig deleteFileConfig;
//	deleteFileConfig.m_fileId = 1008;
//	deleteFileConfig.m_path = deleteFileRoot;
//	deleteFileConfig.m_maxFileBytes = 512;
//	deleteFileConfig.m_maxFileCount = 5;
//	deleteFileConfig.m_checkFileSizeInterval = 1;
//	LogManager::instance().init(deleteFileConfig);
//	for (int32_t i = 0; i < 200; ++i)
//	{
//		LOGINFO_EX(1008, "delete file macro log %d %s", i, "abcdefghijklmnopqrstuvwxyzabcdefghijklmnopqrstuvwxyz");
//	}
//	check(countTopFiles(deleteFileRoot) > 0, "LogManager deleteFile has files before delete");
//	LogManager::instance().deleteFile(1008);
//	check(countTopFiles(deleteFileRoot) == 0, "LogManager deleteFile removes current files");
//
//	std::string normalizeRoot = root + "normalize_config/";
//	LogManagerConfig normalizeConfig;
//	normalizeConfig.m_fileId = 1009;
//	normalizeConfig.m_path = normalizeRoot;
//	normalizeConfig.m_maxFileBytes = 0;
//	normalizeConfig.m_maxFileCount = 1;
//	normalizeConfig.m_checkFileSizeInterval = 0;
//	LogManager::instance().init(normalizeConfig);
//	LOGINFO_EX(1009, "normalize config macro log");
//	LogManager::instance().uninit(1009);
//	check(countToken(readLogText(normalizeRoot), "normalize config macro log") == 1, "LogManager normalize invalid config values");
//
//	std::string relativeRoot = "LogManagerRelativeTest_" + CStringManager::toStringInt32(static_cast<int32_t>(CSystem::currentProcessPid())) + "/";
//	LogManagerConfig relativeConfig;
//	relativeConfig.m_fileId = 1011;
//	relativeConfig.m_path = relativeRoot;
//	relativeConfig.m_maxFileBytes = 1024 * 1024;
//	relativeConfig.m_checkFileSizeInterval = 1;
//	LogManager::instance().init(relativeConfig);
//	LOGINFO_EX(1011, "relative link macro log");
//	LogManager::instance().uninit(1011);
//	if (canCreateSymbolicLink)
//	{
//		std::vector<std::string> relativeFiles = allFiles(relativeRoot);
//		std::string relativeLinkPath;
//		for (size_t i = 0; i < relativeFiles.size(); ++i)
//		{
//			LogManagerOldFile oldFile;
//			if (LogManagerHelper::parseOldLogFile(relativeFiles[i], CSystem::GetCurrentExeName(), oldFile) && oldFile.m_fileIndex < 0)
//			{
//				relativeLinkPath = relativeFiles[i];
//				break;
//			}
//		}
//		std::ifstream relativeInput(relativeLinkPath.c_str(), std::ios::in | std::ios::binary);
//		std::ostringstream relativeText;
//		relativeText << relativeInput.rdbuf();
//		check(countToken(relativeText.str(), "relative link macro log") == 1, "LogManager relative path symbolic link target");
//	}
//	else
//	{
//		skip("LogManager relative path symbolic link target, current process has no symbolic link privilege");
//	}
//
//	std::string switchRoot = root + "write_switch/";
//	LogManagerConfig switchConfig;
//	switchConfig.m_fileId = 1010;
//	switchConfig.m_path = switchRoot;
//	switchConfig.m_maxFileBytes = 1024 * 1024;
//	switchConfig.m_checkFileSizeInterval = 1;
//	LogManager::instance().init(switchConfig);
//	LogManager::instance().set(false, true);
//	LOGINFO_EX(1010, "write switch disabled log");
//	LogManager::instance().set(true, true);
//	LOGINFO_EX(1010, "write switch enabled log");
//	LogManager::instance().uninit(1010);
//	std::string switchText = readLogText(switchRoot);
//	check(countToken(switchText, "write switch disabled log") == 0, "LogManager write switch disabled");
//	check(countToken(switchText, "write switch enabled log") == 1, "LogManager write switch enabled");
//
//	std::string processRoot = root + "process/";
//	LogManagerHelper::createDirectoryRecursive(processRoot);
//	std::string exePath = argc > 0 ? argv[0] : CSystem::GetCurrentExeFullName();
//	std::vector<std::thread> processThreads;
//	std::atomic<int32_t> processFailCount(0);
//	for (int32_t processIndex = 0; processIndex < 4; ++processIndex)
//	{
//		processThreads.push_back(std::thread([exePath, processRoot, &processFailCount]() -> void
//		{
//			std::string command = "\"\"" + exePath + "\" --child \"" + processRoot + "\"\"";
//			int32_t result = std::system(command.c_str());
//			if (result != 0)
//			{
//				processFailCount.fetch_add(1);
//			}
//		}));
//	}
//	for (size_t i = 0; i < processThreads.size(); ++i)
//	{
//		processThreads[i].join();
//	}
//	check(processFailCount.load() == 0, "LogManager multiprocess macro pressure");
//	check(!allFiles(processRoot).empty() && countTopFiles(processRoot) == 0, "LogManager multiprocess output archived on close");
//
//
//#ifdef _WIN32
//	// 归档集成回归：只创建本测试目录和本测试子进程，失败时也负责回收子进程。
//	{
//		const std::string processName = CSystem::GetCurrentExeName();
//		const std::string executable = CSystem::GetCurrentExePath() + CSystem::GetCurrentExeFullName();
//		const std::string suiteRoot = root + "archive_close/";
//		LogManagerHelper::createDirectoryRecursive(suiteRoot);
//		std::vector<PROCESS_INFORMATION> children;
//		std::map<DWORD, std::string> stops;
//		std::map<DWORD, std::string> paths;
//		auto require = [&check](bool ok, const std::string& name) -> void
//		{
//			check(ok, name);
//			if (!ok) { throw std::runtime_error(name); }
//		};
//		auto start = [&](const std::string& path, const std::string& mode) -> DWORD
//		{
//			LogManagerHelper::createDirectoryRecursive(path);
//			std::ostringstream ticket;
//			ticket << suiteRoot << "child_" << children.size();
//			std::string ready = ticket.str() + ".ready";
//			std::string stop = ticket.str() + ".stop";
//			std::string command = "\"" + executable + "\" --archive-child \"" + path + "\" " + mode +
//				" \"" + ready + "\" \"" + stop + "\"";
//			std::vector<char> buffer(command.begin(), command.end());
//			buffer.push_back('\0');
//			STARTUPINFOA startup = {};
//			startup.cb = sizeof(startup);
//			PROCESS_INFORMATION child = {};
//			require(CreateProcessA(executable.c_str(), &buffer[0], nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
//				nullptr, nullptr, &startup, &child) != FALSE, "archive child starts");
//			CloseHandle(child.hThread);
//			child.hThread = nullptr;
//			children.push_back(child);
//			stops[child.dwProcessId] = stop;
//			paths[child.dwProcessId] = path;
//			auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
//			while (!LogManagerHelper::exist(ready))
//			{
//				if (WaitForSingleObject(child.hProcess, 0) != WAIT_TIMEOUT) { throw std::runtime_error("archive child exited before ready"); }
//				if (std::chrono::steady_clock::now() >= deadline) { throw std::runtime_error("archive child ready timeout"); }
//				std::this_thread::sleep_for(std::chrono::milliseconds(25));
//			}
//			require(true, "archive child ready within timeout");
//			return child.dwProcessId;
//		};
//		auto finish = [&](DWORD pid) -> void
//		{
//			std::ofstream(stops.at(pid).c_str()) << "stop";
//			for (size_t i = 0; i < children.size(); ++i)
//			{
//				if (children[i].dwProcessId != pid) { continue; }
//				require(WaitForSingleObject(children[i].hProcess, 10000) == WAIT_OBJECT_0, "archive child exits within timeout");
//				DWORD code = 0;
//				require(GetExitCodeProcess(children[i].hProcess, &code) && code == 0, "archive child exit status");
//				return;
//			}
//			throw std::runtime_error("unknown archive child");
//		};
//		auto ownFiles = [&](DWORD pid) -> std::vector<std::string>
//		{
//			std::vector<std::string> result;
//			std::vector<std::string> files = LogManagerHelper::listTopFiles(paths.at(pid));
//			for (size_t i = 0; i < files.size(); ++i)
//			{
//				LogManagerOldFile info;
//				if (LogManagerHelper::parseOldLogFile(files[i], processName, info) &&
//					info.m_pid == static_cast<int32_t>(pid)) { result.push_back(files[i]); }
//			}
//			return result;
//		};
//		auto entityCount = [&](const std::vector<std::string>& files) -> size_t
//		{
//			size_t count = 0;
//			for (size_t i = 0; i < files.size(); ++i)
//			{
//				LogManagerOldFile info;
//				if (LogManagerHelper::parseOldLogFile(files[i], processName, info) && info.m_fileIndex >= 0) { ++count; }
//			}
//			return count;
//		};
//		auto snapshot = [&](DWORD pid) -> std::map<std::string, std::string>
//		{
//			std::map<std::string, std::string> result;
//			std::vector<std::string> files = ownFiles(pid);
//			for (size_t i = 0; i < files.size(); ++i) { result[files[i]] = readFile(files[i]); }
//			return result;
//		};
//		auto unchanged = [&](const std::map<std::string, std::string>& saved, const std::string& name) -> void
//		{
//			for (auto it = saved.begin(); it != saved.end(); ++it)
//			{
//				require(LogManagerHelper::exist(it->first) && readFile(it->first) == it->second, name);
//			}
//		};
//		auto verifyArchive = [&](DWORD pid, size_t expectedSessions) -> std::string
//		{
//			require(ownFiles(pid).empty(), "normal close removes only own top-level logs");
//			std::vector<std::string> dirs = directories(paths.at(pid) + "/" + processName + "_log");
//			size_t sessions = 0;
//			std::string combined;
//			for (size_t d = 0; d < dirs.size(); ++d)
//			{
//				std::vector<std::string> files = LogManagerHelper::listTopFiles(dirs[d]);
//				std::string text, link, latest;
//				int32_t maxIndex = -1;
//				size_t count = 0, zero = 0, links = 0;
//				for (size_t f = 0; f < files.size(); ++f)
//				{
//					LogManagerOldFile info;
//					if (!LogManagerHelper::parseOldLogFile(files[f], processName, info) || info.m_pid != static_cast<int32_t>(pid)) { continue; }
//					if (info.m_fileIndex < 0) { ++links; link = files[f]; continue; }
//					++count;
//					if (info.m_fileIndex == 0) { ++zero; }
//					if (info.m_fileIndex > maxIndex) { maxIndex = info.m_fileIndex; latest = files[f]; }
//					text += readFile(files[f]);
//				}
//				if (count == 0) { continue; }
//				++sessions;
//				require(count <= 3 && zero == 1, "archive retains zero and rolling file limit");
//				require(text.find("BEGIN") != std::string::npos && text.find("END") != std::string::npos, "BEGIN and END archived");
//				require(links == 1, "owned symbolic link archived");
//				require(readFile(link) == readFile(latest), "archived relative link resolves to latest file");
//				combined += text;
//			}
//			require(sessions == expectedSessions, "separate archive directory per session");
//			return combined;
//		};
//		try
//		{
//			const char* modes[] = { "explicit", "destructor", "reinit", "multi", "noarchive", "delete" };
//			for (size_t m = 0; m < sizeof(modes) / sizeof(modes[0]); ++m)
//			{
//				std::string mode = modes[m], path = suiteRoot + mode;
//				LogManagerHelper::createDirectoryRecursive(path);
//				std::string old = path + "/" + processName + "_123_20000101_010101.0.log";
//				std::ofstream(old.c_str()) << "untouched crash evidence";
//				DWORD pid = start(path, mode);
//				require(readFile(old) == "untouched crash evidence", mode + ": startup preserves residual log");
//				size_t expected = mode == "multi" ? 2 : 1;
//				require(entityCount(ownFiles(pid)) == 3 * expected, mode + ": live rolling files remain at top level");
//				require(!LogManagerHelper::exist(path + "/" + processName + "_log"), mode + ": no archive before close");
//				finish(pid);
//				require(readFile(old) == "untouched crash evidence", mode + ": close preserves residual log");
//				if (mode == "noarchive")
//				{
//					require(entityCount(ownFiles(pid)) == 3, "disabled archive leaves files in place");
//					require(!LogManagerHelper::exist(path + "/" + processName + "_log"), "disabled archive creates no history");
//				}
//				else if (mode == "delete")
//				{
//					require(ownFiles(pid).empty(), "deleteFile removes owned files");
//					require(!LogManagerHelper::exist(path + "/" + processName + "_log"), "deleteFile does not archive");
//				}
//				else
//				{
//					std::string text = verifyArchive(pid, mode == "reinit" ? 2 : expected);
//					require(text.find("LAST17") != std::string::npos, "last message retained");
//					if (mode == "reinit") { require(text.find("SECOND_SESSION") != std::string::npos, "reopened session retained"); }
//					if (mode == "multi") { require(text.find("LAST18") != std::string::npos, "second ID writable after first closes"); }
//				}
//			}
//			std::string shared = suiteRoot + "multiple_processes";
//			DWORD first = start(shared, "explicit");
//			auto firstSnapshot = snapshot(first);
//			DWORD second = start(shared, "explicit");
//			unchanged(firstSnapshot, "second startup preserves first process files");
//			auto secondSnapshot = snapshot(second);
//			finish(first);
//			verifyArchive(first, 1);
//			unchanged(secondSnapshot, "first close preserves second process files");
//			finish(second);
//			verifyArchive(second, 1);
//			std::string collision = suiteRoot + "collision";
//			DWORD pid = start(collision, "explicit");
//			std::vector<std::string> files = ownFiles(pid);
//			std::string zero;
//			LogManagerOldFile info;
//			for (size_t i = 0; i < files.size(); ++i)
//			{
//				if (LogManagerHelper::parseOldLogFile(files[i], processName, info) && info.m_fileIndex == 0) { zero = files[i]; break; }
//			}
//			require(!zero.empty(), "collision setup has first entity");
//			std::string original = readFile(zero);
//			std::string foreign = collision + "/" + info.m_baseName + ".999999.log";
//			std::ofstream(foreign.c_str()) << "not created by logger";
//			std::string archive = collision + "/" + processName + "_log/" + info.m_date + "_" + info.m_time + "_" + CStringManager::toStringInt32(info.m_pid);
//			LogManagerHelper::createDirectoryRecursive(archive);
//			std::string target = archive + "/" + info.m_fileName;
//			std::ofstream(target.c_str()) << "existing archive";
//			finish(pid);
//			require(readFile(foreign) == "not created by logger", "unowned matching-name file not moved");
//			require(readFile(target) == "existing archive", "target collision does not overwrite");
//			require(readFile(zero) == original, "failed move retains source bytes");
//			std::string blocked = suiteRoot + "blocked";
//			pid = start(blocked, "explicit");
//			std::ofstream((blocked + "/" + processName + "_log").c_str()) << "directory blocker";
//			finish(pid);
//			require(entityCount(ownFiles(pid)) == 3, "archive directory failure preserves sources");
//			std::string abnormal = suiteRoot + "abnormal_exit";
//			DWORD crashed = start(abnormal, "explicit");
//			for (size_t i = 0; i < children.size(); ++i)
//			{
//				if (children[i].dwProcessId == crashed)
//				{
//					require(TerminateProcess(children[i].hProcess, 23) != FALSE, "terminate own test child");
//					require(WaitForSingleObject(children[i].hProcess, 5000) == WAIT_OBJECT_0, "abnormal child stopped");
//				}
//			}
//			auto crashSnapshot = snapshot(crashed);
//			require(crashSnapshot.size() == 4, "abnormal exit leaves entities and link");
//			DWORD next = start(abnormal, "explicit");
//			finish(next);
//			verifyArchive(next, 1);
//			unchanged(crashSnapshot, "later startup and shutdown preserve crash evidence");
//		}
//		catch (const std::exception& exception)
//		{
//			check(false, std::string("archive suite exception: ") + exception.what());
//		}
//		for (size_t i = 0; i < children.size(); ++i)
//		{
//			if (WaitForSingleObject(children[i].hProcess, 0) == WAIT_TIMEOUT)
//			{
//				TerminateProcess(children[i].hProcess, 24);
//				WaitForSingleObject(children[i].hProcess, 5000);
//			}
//			CloseHandle(children[i].hProcess);
//		}
//	}
//#else
//	skip("close-time archive child-process regression currently requires Windows");
//#endif
//	LogManager::instance().uninitAll();
//	std::cout << "LogManager test " << (failCount == 0 ? "PASS" : "FAIL") << ", total=" << totalCount << ", failed=" << failCount << ", skipped=" << skipCount << std::endl;
//	return failCount == 0 ? 0 : 1;
//}
//