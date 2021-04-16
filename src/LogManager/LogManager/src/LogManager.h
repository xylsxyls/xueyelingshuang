#pragma once
#include "LogManagerMacro.h"
#include <string>
#include <stdint.h>
#include <map>
#include <atomic>

//LOGDEBUG在release下不会输进日志文件
#define LOGDEBUG(format, ...) LogManager::instance().print(0, LogManager::LOG_DEBUG, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)
#define LOGINFO(format, ...) LogManager::instance().print(0, LogManager::LOG_INFO, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)
#define LOGWARNING(format, ...) LogManager::instance().print(0, LogManager::LOG_WARNING, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)
#define LOGERROR(format, ...) LogManager::instance().print(0, LogManager::LOG_ERROR, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)
#define LOGFATAL(format, ...) LogManager::instance().print(0, LogManager::LOG_FATAL, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)

#define LOGDEBUG_EX(fildId, format, ...) LogManager::instance().print(fildId, LogManager::LOG_DEBUG, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)
#define LOGINFO_EX(fildId, format, ...) LogManager::instance().print(fildId, LogManager::LOG_INFO, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)
#define LOGWARNING_EX(fildId, format, ...) LogManager::instance().print(fildId, LogManager::LOG_WARNING, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)
#define LOGERROR_EX(fildId, format, ...) LogManager::instance().print(fildId, LogManager::LOG_ERROR, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)
#define LOGFATAL_EX(fildId, format, ...) LogManager::instance().print(fildId, LogManager::LOG_FATAL, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)

//begin和end是内部使用
#define LOGBEGIN(fildId, format, ...) LogManager::instance().print(fildId, LogManager::LOG_BEGIN, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)
#define LOGEND(fildId, format, ...) LogManager::instance().print(fildId, LogManager::LOG_END, __FILE__, __FUNCTION__, "", "", 0, format, ##__VA_ARGS__)

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
	//初始化和反初始化不能和printf并行
	void init(int32_t fileId = 0, const std::string& path = "");

	//该接口会动态应用到所有日志文件
	void set(bool writeLog, bool writeBeginEnd);

	void print(int32_t fileId, LogLevel flag, const std::string& fileMacro, const std::string& funName, const std::string& exeName, const std::string& intDateTime, int32_t threadId, const char* format, ...);

	void uninit(int32_t fileId);

	void uninitAll();

	void deleteFile(int32_t fileId);

	int32_t findFileId(const std::string& path);

protected:
	std::ofstream* getLogFile(int32_t fileId);

	std::string getLogPath(int32_t fileId);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_exeName;
	std::map<int32_t, std::pair<std::string, std::ofstream*>> m_logMap;
	std::atomic<bool> m_writeBeginEnd;
	std::atomic<bool> m_writeLog;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	ProcessReadWriteMutex* m_processMutex;
};