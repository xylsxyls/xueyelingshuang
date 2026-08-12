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
m_preparedLogDirMap(),
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
		deletePaths.push_back(logFile->m_linkPath);
		for (int32_t fileIndex = 0; fileIndex <= logFile->m_currentIndex; ++fileIndex)
		{
			deletePaths.push_back(LogManagerHelper::buildEntityPath(logFile->m_logDir, logFile->m_baseName, fileIndex));
		}
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

	prepareOldLogNoLock(logDir, realConfig.m_archiveOldLog);

	LogManagerFile* logFile = new (std::nothrow) LogManagerFile;
	if (logFile == nullptr)
	{
		return nullptr;
	}
	logFile->m_config = realConfig;
	logFile->m_logDir = logDir;
	std::string timeName = LogManagerHelper::currentTimeName();
	logFile->m_baseName = LogManagerHelper::buildBaseName(m_processName, static_cast<int32_t>(CSystem::currentProcessPid()), timeName);
	logFile->m_linkPath = LogManagerHelper::buildLinkPath(logDir, logFile->m_baseName);

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
	if (LogManagerHelper::fileSize(logFile->m_currentEntityPath) > logFile->m_config.m_maxFileBytes)
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

	logFile->m_currentIndex = fileIndex;
	logFile->m_currentEntityPath = LogManagerHelper::buildEntityPath(logFile->m_logDir, logFile->m_baseName, fileIndex);
	logFile->m_logFile = new (std::nothrow) std::ofstream(logFile->m_currentEntityPath.c_str(), std::ios::out | std::ios::app);
	if (logFile->m_logFile == nullptr || !logFile->m_logFile->is_open())
	{
		if (logFile->m_logFile != nullptr)
		{
			delete logFile->m_logFile;
			logFile->m_logFile = nullptr;
		}
		return false;
	}
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
	if (LogManagerHelper::createSymbolicLinkFile(logFile->m_linkPath, LogManagerHelper::fileName(logFile->m_currentEntityPath)))
	{
		logFile->m_linkFileLock = LogManagerHelper::lockFileDelete(logFile->m_linkPath, true);
	}
}

void LogManager::prepareOldLogNoLock(const std::string& logDir, bool archiveOldLog)
{
	std::string preparedKey = logDir + "|" + m_processName;
	if (m_preparedLogDirMap.find(preparedKey) != m_preparedLogDirMap.end())
	{
		return;
	}
	m_preparedLogDirMap[preparedKey] = true;

	std::vector<std::string> files = LogManagerHelper::listTopFiles(logDir);
	for (size_t i = 0; i < files.size(); ++i)
	{
		LogManagerOldFile oldFile;
		if (!LogManagerHelper::parseOldLogFile(files[i], m_processName, oldFile))
		{
			continue;
		}
		if (!archiveOldLog)
		{
			LogManagerHelper::deleteFile(oldFile.m_filePath);
			continue;
		}

		std::string archiveRoot = logDir + m_processName + "_log";
		LogManagerHelper::createDirectoryRecursive(archiveRoot);
		std::string archiveDir = archiveRoot + "/" + oldFile.m_date + "_" + oldFile.m_time + "_" + CStringManager::toStringInt32(oldFile.m_pid);
		LogManagerHelper::createDirectoryRecursive(archiveDir);
		std::string targetPath = LogManagerHelper::normalizeLogDir(archiveDir) + oldFile.m_fileName;
		LogManagerHelper::renameFile(oldFile.m_filePath, targetPath);
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
		LogManagerHelper::deleteFile(LogManagerHelper::buildEntityPath(logFile->m_logDir, logFile->m_baseName, fileIndex));
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
#elif __unix__
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
#elif __unix__
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

//#include <atomic>
//#include <cstdlib>
//#include <functional>
//#include <iostream>
//#include <sstream>
//#include <thread>
//
//#ifdef _WIN32
//#include <process.h>
//#endif
//
//int main(int32_t argc, char** argv)
//{
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
//	std::function<int32_t(const std::string&)> countTopFiles = [](const std::string& path) -> int32_t
//	{
//		return static_cast<int32_t>(LogManagerHelper::listTopFiles(path).size());
//	};
//	std::function<std::string(const std::string&)> readLogText = [](const std::string& path) -> std::string
//	{
//		std::ostringstream oss;
//		std::vector<std::string> files = LogManagerHelper::listTopFiles(path);
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
//	check(LogManagerHelper::exist(archiveRoot + CSystem::GetCurrentExeName() + "_log/20260809_034122_456/" + archiveOldName), "LogManager archive old log directory");
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
//	check(!LogManagerHelper::exist(deleteRoot + oldName), "LogManager delete old log");
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
//		std::vector<std::string> relativeFiles = LogManagerHelper::listTopFiles(relativeRoot);
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
//	check(countTopFiles(processRoot) > 0, "LogManager multiprocess file output");
//
//	LogManager::instance().uninitAll();
//	std::cout << "LogManager test " << (failCount == 0 ? "PASS" : "FAIL") << ", total=" << totalCount << ", failed=" << failCount << ", skipped=" << skipCount << std::endl;
//	return failCount == 0 ? 0 : 1;
//}