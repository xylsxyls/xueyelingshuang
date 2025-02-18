﻿#include "LogManager.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include <fstream>
#include <stdarg.h>

LogManager::LogManager():
m_writeMutex(nullptr),
m_writeLog(true),
m_writeBeginEnd(true),
m_isProcessMutex(false)
{
	m_exeName = CSystem::GetCurrentExeFullName();
	m_writeMutex = new ReadWriteMutex;
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

void LogManager::init(int32_t fileId, const std::string& path)
{
	if (!m_writeLog)
	{
		return;
	}
	
	if (m_logMap.find(fileId) == m_logMap.end())
	{
		std::string logPath;
		if (path.empty())
		{
			logPath = CSystem::GetCurrentExePath() + m_exeName + ".log";
		}
		else
		{
			logPath = path;
		}
		std::ofstream* logFile = new std::ofstream(logPath.c_str(), std::ios::app);
		if (logFile == nullptr || !logFile->is_open())
		{
			return;
		}
		m_logMap[fileId] = std::pair<std::string, std::ofstream*>(path, logFile);
		if (m_writeBeginEnd)
		{
			LOGBEGIN(fileId, "");
		}
	}
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

	m_isProcessMutex = isProcessMutex;
	if (m_isProcessMutex)
	{
		delete m_writeMutex;
		m_writeMutex = new ProcessReadWriteMutex("LogManager_Mutex");
	}
	else
	{
		delete m_writeMutex;
		m_writeMutex = new ReadWriteMutex;
	}
}

void LogManager::print(int32_t fileId, LogLevel flag, const std::string& fileMacro, const std::string& funName, const std::string& exeName, const std::string& intDateTime, int32_t threadId, const char* format, ...)
{
	if (m_writeMutex == nullptr || !m_writeLog)
	{
		return;
	}

	if ((fileId == 0) && (m_logMap.find(0) == m_logMap.end()))
	{
		init(0, "");
	}

	std::ofstream* logFile = getLogFile(fileId);
	if (logFile == nullptr)
	{
		return;
	}

	std::string strFlag;
	switch (flag)
	{
	case LogManager::LOG_DEBUG:
	{
#ifdef _DEBUG
		strFlag = "  DEBUG";
		break;
#else
		return;
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
		break;
	}

	std::string str;
	
	va_list args;
	va_start(args, format);
#ifdef _WIN32
	int size = _vscprintf(format, args);
#elif __unix__
	va_list argcopy;
	va_copy(argcopy, args);
	int size = vsnprintf(nullptr, 0, format, argcopy);
#endif
	//?resize分配后string类会自动在最后分配\0，resize(5)则总长6
	str.resize(size);
	if (size != 0)
	{
#ifdef _WIN32
		//?即便分配了足够内存，长度必须加1，否则会崩溃
		vsprintf_s(&str[0], size + 1, format, args);
#elif __unix__
		vsnprintf(&str[0], size + 1, format, args);
#endif
	}
	va_end(args);

	std::string fileMacroTemp;
	size_t nRight = fileMacro.find_last_of("/\\");
	fileMacroTemp = CStringManager::Mid(fileMacro, nRight + 1, fileMacro.length() - nRight - 1);
	
	WriteLock writelock(*m_writeMutex);
	if (flag == LOG_BEGIN && m_writeBeginEnd)
	{
		bool isFileEmpty = true;
		std::ofstream* newLogFile = new std::ofstream(getLogPath(fileId), std::ios::in | std::ios::out | std::ios::app);
		newLogFile->seekp(std::ios::beg);
		if (newLogFile->rdbuf()->sgetc() != std::char_traits<char>::eof())
		{
			isFileEmpty = false;
		}
		delete newLogFile;
		if (isFileEmpty == false)
		{
			*logFile << std::endl;
		}
	}
	std::string beginEnd;
	//?这里str就是要打印的日志
	*logFile << "[" + (intDateTime.empty() ? IntDateTime().timeToString() : intDateTime) + "][" + strFlag + "][ThreadId:" << (threadId == 0 ? CSystem::SystemThreadId() : threadId) << "][" << (exeName.empty() ? m_exeName : exeName) << "][" << fileMacroTemp << "][" << funName.c_str() << "]" << ((flag == LOG_BEGIN || flag == LOG_END) ? beginEnd : beginEnd = " : " + str) << std::endl;
}

void LogManager::uninit(int32_t fileId)
{
	if (m_writeBeginEnd)
	{
		LOGEND(fileId, "");
	}
	auto itLog = m_logMap.find(fileId);
	if (itLog == m_logMap.end())
	{
		return;
	}
	std::ofstream* logFile = itLog->second.second;
	m_logMap.erase(itLog);
	delete logFile;
}

void LogManager::uninitAll()
{
	std::vector<int32_t> vecLogId;
	for (auto itLog = m_logMap.begin(); itLog != m_logMap.end(); ++itLog)
	{
		vecLogId.push_back(itLog->first);
	}
	int32_t index = -1;
	while (index++ != vecLogId.size() - 1)
	{
		uninit(vecLogId[index]);
	}
}

void LogManager::deleteFile(int32_t fileId)
{
	std::string logPath = getLogPath(fileId);
	uninit(fileId);
	CSystem::deleteFile(logPath.c_str());
}

int32_t LogManager::findFileId(const std::string& path)
{
	for (auto itLog = m_logMap.begin(); itLog != m_logMap.end(); ++itLog)
	{
		if (itLog->second.first == path)
		{
			return itLog->first;
		}
	}
	return -1;
}

std::ofstream* LogManager::getLogFile(int32_t fileId)
{
	auto itLog = m_logMap.find(fileId);
	if (itLog == m_logMap.end())
	{
		return nullptr;
	}
	return itLog->second.second;
}

std::string LogManager::getLogPath(int32_t fileId)
{
	auto itLog = m_logMap.find(fileId);
	if (itLog == m_logMap.end())
	{
		return "";
	}
	return itLog->second.first;
}