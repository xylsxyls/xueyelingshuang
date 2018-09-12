#pragma once
#include "LogManagerMacro.h"
#include <string>
#include <stdint.h>

//LOGDEBUG在release下不会输进日志文件
#define LOGDEBUG(format, ...) LogManager::instance().print(LogManager::LOG_DEBUG, __FILE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOGINFO(format, ...) LogManager::instance().print(LogManager::LOG_INFO, __FILE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOGWARNING(format, ...) LogManager::instance().print(LogManager::LOG_WARNING, __FILE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOGERROR(format, ...) LogManager::instance().print(LogManager::LOG_ERROR, __FILE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOGFATAL(format, ...) LogManager::instance().print(LogManager::LOG_FATAL, __FILE__, __FUNCTION__, format, ##__VA_ARGS__)
//begin和end是内部使用
#define LOGBEGIN(format, ...) LogManager::instance().print(LogManager::LOG_BEGIN, __FILE__, __FUNCTION__, format, ##__VA_ARGS__)
#define LOGEND(format, ...) LogManager::instance().print(LogManager::LOG_END, __FILE__, __FUNCTION__, format, ##__VA_ARGS__)

namespace std
{
	typedef basic_ofstream<char, char_traits<char> > ofstream;
};
class ProcessReadWriteMutex;

class LogManagerAPI LogManager
{
public:
	enum LogLevel
	{
		LOG_DEBUG = 0x00000001,
		LOG_INFO = 0x00000010,
		LOG_WARNING = 0x00000100,
		LOG_ERROR = 0x00001000,
		LOG_FATAL = 0x00010000,
		LOG_BEGIN = 0x00100000,
		LOG_END = 0x01000000
	};
private:
	LogManager();

	~LogManager();

public:
	static LogManager& instance();

public:
	void init(const std::string& path = "");

	void print(LogLevel flag, const std::string& fileMacro, const std::string& funName, const char* format, ...);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_exeName;
	std::string m_logPath;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	std::ofstream* m_log;
	ProcessReadWriteMutex* m_processMutex;
};