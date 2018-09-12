#include "LogManager.h"
#include "CGetPath/CGetPathAPI.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include <fstream>

LogManager::LogManager():
m_log(nullptr),
m_processMutex(nullptr)
{
	m_exeName = CGetPath::GetCurrentExeName();
	m_logPath = CGetPath::GetCurrentExePath() + m_exeName + ".log";
	m_exeName.append(".exe");
	m_processMutex = new ProcessReadWriteMutex("LogManager_Mutex");
}

LogManager::~LogManager()
{
	if (m_log != nullptr)
	{
		LOGEND("");
		delete m_log;
		m_log = nullptr;
	}
	if (m_processMutex != nullptr)
	{
		delete m_processMutex;
		m_processMutex = nullptr;
	}
}

LogManager& LogManager::instance()
{
	static LogManager s_logManager;
	return s_logManager;
}

void LogManager::init(const std::string& path)
{
	if (!path.empty())
	{
		m_logPath = path;
	}
	m_log = new std::ofstream(m_logPath.c_str(), std::ios::app);
	LOGBEGIN("");
}

void LogManager::print(LogLevel flag, const string& fileMacro, const std::string& funName, const char* format, ...)
{
	if (m_log == nullptr || m_processMutex == nullptr)
	{
		init();
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
	string str;
	va_list args = nullptr;
	va_start(args, format);
	int32_t size = ::_vscprintf(format, args);
	//?resize分配后string类会自动在最后分配\0，resize(5)则总长6
	str.resize(size);
	//?即便分配了足够内存，长度必须加1，否则会崩溃
	::vsprintf_s(&str[0], size + 1, format, args);
	va_end(args);
	string fileMacroTemp;
	int32_t nRight = fileMacro.find_last_of("/\\");
	fileMacroTemp = CStringManager::Mid(fileMacro, nRight + 1, fileMacro.length() - nRight - 1);
	
	
	WriteLock writelock(*m_processMutex);
	if (flag == LOG_BEGIN)
	{
		bool isFileEmpty = true;
		std::ofstream* log = new std::ofstream(m_logPath, std::ios::in | std::ios::out | std::ios::app);
		log->seekp(ios::beg);
		if (log->rdbuf()->sgetc() != char_traits<char>::eof())
		{
			isFileEmpty = false;
		}
		delete log;
		if (isFileEmpty == false)
		{
			*m_log << std::endl;
		}
	}
	std::string beginEnd;
	//?这里str就是要打印的日志
	*m_log << "[" + IntDateTime().timeToString() + "][" + strFlag + "][ThreadId:" << CSystem::SystemThreadId() << "][" << m_exeName << "][" << fileMacroTemp << "][" << funName.c_str() << "]" << ((flag == LOG_BEGIN || flag == LOG_END) ? beginEnd : beginEnd = " : " + str) << std::endl;
}