#pragma once
#ifdef _WIN32
#define _LogSenderAPI
#endif

#ifdef _MSC_VER
    #ifdef _LogSenderAPI
        #define LogSenderAPI _declspec(dllimport)
    #else
        #define LogSenderAPI _declspec(dllexport)
    #endif
#else
    #if defined(__GNUC__) && __GNUC__ >= 4
        #define LogSenderAPI __attribute__ ((visibility("default")))
    #else
        #define LogSenderAPI
    #endif
#endif

#include <stdint.h>
#include <mutex>
#include <string>

#ifdef _MSC_VER
struct __declspec(novtable) LogPackage
#elif __unix__
struct LogPackage
#endif
{
public:
	enum LogLevel
	{
		LOG_DEBUG = 0x00000001,
		LOG_INFO = 0x00000010,
		LOG_WARNING = 0x00000100,
		LOG_ERROR = 0x00001000,
		LOG_FATAL = 0x00010000,
	};

	//人员ID
	int32_t m_peopleId;
	//日志等级
	LogLevel m_logLevel;
	//是否向网络发送
	bool m_isSendNet;
	//是否向屏幕发送
	bool m_isSendScreen;
	//是否写入日志
	bool m_isWriteLog;
	//输出日志的文件名
	const char* m_fileName;
	//输出日志的函数名
	const char* m_funName;
	//日志文件的文件名
	const char* m_logName;

	/** 构造函数
	*/
	LogPackage(int32_t peopleId,
		LogLevel logLevel,
		bool isSendNet,
		bool isSendScreen,
		bool isWriteLog,
		const char* fileName,
		const char* funName,
		const char* logName)
	{
		m_peopleId = peopleId;
		m_logLevel = logLevel;
		m_isSendNet = isSendNet;
		m_isSendScreen = isSendScreen;
		m_isWriteLog = isWriteLog;
		m_fileName = fileName;
		m_funName = funName;
		m_logName = logName;
	}
};

class ProtoMessage;

const std::string LOGTEST_CLIENT_VERSION = "1.2";

class LogSenderAPI LogSenderInterface
{
public:
	virtual void logTestUniqueOpen() = 0;

	virtual void logTestUniqueClose() = 0;

	virtual bool logTestExist() = 0;

	virtual void logTestOpen() = 0;

	virtual void logSend(const LogPackage& package, const char* format, ...) = 0;

	virtual void logTestClose() = 0;

	virtual void set(bool dealLog) = 0;
};

extern "C"
{
	LogSenderAPI LogSenderInterface* logInstance();
}

class LogSenderAPI LogSender : public LogSenderInterface
{
protected:
	LogSender();

public:
	static LogSender& instance();

public:
	void logTestUniqueOpen();

	void logTestUniqueClose();

	bool logTestExist();

	void logTestOpen();

	void logSend(const LogPackage& package, const char* format, ...);

	void logTestClose();

	void set(bool dealLog);

protected:
	void send(const std::string& message, int32_t type);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	uint32_t m_processPid;
	std::string m_processName;
	std::string m_loginName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};

#ifdef __unix__
#include <unistd.h>
#include <dlfcn.h>
#endif
#ifdef _MSC_VER
#include <windows.h>
#endif

#define LOG_TEST_UNIQUE_OPEN if (LogSenderManager::instance().getInterface() != nullptr) LogSenderManager::instance().getInterface()->logTestUniqueOpen
#define LOG_TEST_UNIQUE_CLOSE if (LogSenderManager::instance().getInterface() != nullptr) LogSenderManager::instance().getInterface()->logTestUniqueClose
#define LOG_TEST_EXIST if (LogSenderManager::instance().getInterface() != nullptr) LogSenderManager::instance().getInterface()->logTestExist
#define LOG_TEST_OPEN if (LogSenderManager::instance().getInterface() != nullptr) LogSenderManager::instance().getInterface()->logTestOpen
#define LOG_TEST_CLOSE if (LogSenderManager::instance().getInterface() != nullptr) LogSenderManager::instance().getInterface()->logTestClose
//是否处理日志
#define LOG_TEST_SET(dealLog) if (LogSenderManager::instance().getInterface() != nullptr) LogSenderManager::instance().getInterface()->set(dealLog)

#define LOG_NAME_SET(name) LogSenderManager::instance().setLogName(name)

//第一次发送日志必须单线程完成，发送模块单一实例的初始化必须单线程完成
//LogTest文件记录，MessageTest显示，发送网络
#define LOG_SEND(format, ...) LOG_SEND_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_DEBUG(format, ...) LOG_SEND_DEBUG_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_INFO(format, ...) LOG_SEND_INFO_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_WARNING(format, ...) LOG_SEND_WARNING_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_ERROR(format, ...) LOG_SEND_ERROR_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_FATAL(format, ...) LOG_SEND_FATAL_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)

#define LOG_SEND_PEOPLE(peopleId, format, ...) LOG_SEND_PEOPLE_EX(LogSenderManager::instance().logName().c_str(), peopleId, format, ##__VA_ARGS__)
#define LOG_SEND_PEOPLE_DEBUG(peopleId, format, ...) LOG_SEND_PEOPLE_DEBUG_EX(LogSenderManager::instance().logName().c_str(), peopleId, format, ##__VA_ARGS__)
#define LOG_SEND_PEOPLE_INFO(peopleId, format, ...) LOG_SEND_PEOPLE_INFO_EX(LogSenderManager::instance().logName().c_str(), peopleId, format, ##__VA_ARGS__)
#define LOG_SEND_PEOPLE_WARNING(peopleId, format, ...) LOG_SEND_PEOPLE_WARNING_EX(LogSenderManager::instance().logName().c_str(), peopleId, format, ##__VA_ARGS__)
#define LOG_SEND_PEOPLE_ERROR(peopleId, format, ...) LOG_SEND_PEOPLE_ERROR_EX(LogSenderManager::instance().logName().c_str(), peopleId, format, ##__VA_ARGS__)
#define LOG_SEND_PEOPLE_FATAL(peopleId, format, ...) LOG_SEND_PEOPLE_FATAL_EX(LogSenderManager::instance().logName().c_str(), peopleId, format, ##__VA_ARGS__)

//LogTest文件记录，MessageTest显示，不发送网络
#define LOG_SEND_LOCAL(format, ...) LOG_SEND_LOCAL_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_LOCAL_DEBUG(format, ...) LOG_SEND_LOCAL_DEBUG_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_LOCAL_INFO(format, ...) LOG_SEND_LOCAL_INFO_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_LOCAL_WARNING(format, ...) LOG_SEND_LOCAL_WARNING_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_LOCAL_ERROR(format, ...) LOG_SEND_LOCAL_ERROR_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_LOCAL_FATAL(format, ...) LOG_SEND_LOCAL_FATAL_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)

#define LOG_SEND_PEOPLE_LOCAL(peopleId, format, ...) LOG_SEND_PEOPLE_LOCAL_EX(LogSenderManager::instance().logName().c_str(), peopleId, format, ##__VA_ARGS__)
#define LOG_SEND_PEOPLE_LOCAL_DEBUG(peopleId, format, ...) LOG_SEND_PEOPLE_LOCAL_DEBUG_EX(LogSenderManager::instance().logName().c_str(), peopleId, format, ##__VA_ARGS__)
#define LOG_SEND_PEOPLE_LOCAL_INFO(peopleId, format, ...) LOG_SEND_PEOPLE_LOCAL_INFO_EX(LogSenderManager::instance().logName().c_str(), peopleId, format, ##__VA_ARGS__)
#define LOG_SEND_PEOPLE_LOCAL_WARNING(peopleId, format, ...) LOG_SEND_PEOPLE_LOCAL_WARNING_EX(LogSenderManager::instance().logName().c_str(), peopleId, format, ##__VA_ARGS__)
#define LOG_SEND_PEOPLE_LOCAL_ERROR(peopleId, format, ...) LOG_SEND_PEOPLE_LOCAL_ERROR_EX(LogSenderManager::instance().logName().c_str(), peopleId, format, ##__VA_ARGS__)
#define LOG_SEND_PEOPLE_LOCAL_FATAL(peopleId, format, ...) LOG_SEND_PEOPLE_LOCAL_FATAL_EX(LogSenderManager::instance().logName().c_str(), peopleId, format, ##__VA_ARGS__)

//只通过LogTest记录到文件
#define LOG_SEND_FILE(format, ...) LOG_SEND_FILE_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_FILE_DEBUG(format, ...) LOG_SEND_FILE_DEBUG_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_FILE_INFO(format, ...) LOG_SEND_FILE_INFO_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_FILE_WARNING(format, ...) LOG_SEND_FILE_WARNING_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_FILE_ERROR(format, ...) LOG_SEND_FILE_ERROR_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_FILE_FATAL(format, ...) LOG_SEND_FILE_FATAL_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)

#define LOG_SEND_PEOPLE_FILE(peopleId, format, ...) LOG_SEND_PEOPLE_FILE_EX(LogSenderManager::instance().logName().c_str(), peopleId, format, ##__VA_ARGS__)
#define LOG_SEND_PEOPLE_FILE_DEBUG(peopleId, format, ...) LOG_SEND_PEOPLE_FILE_DEBUG_EX(LogSenderManager::instance().logName().c_str(), peopleId, format, ##__VA_ARGS__)
#define LOG_SEND_PEOPLE_FILE_INFO(peopleId, format, ...) LOG_SEND_PEOPLE_FILE_INFO_EX(LogSenderManager::instance().logName().c_str(), peopleId, format, ##__VA_ARGS__)
#define LOG_SEND_PEOPLE_FILE_WARNING(peopleId, format, ...) LOG_SEND_PEOPLE_FILE_WARNING_EX(LogSenderManager::instance().logName().c_str(), peopleId, format, ##__VA_ARGS__)
#define LOG_SEND_PEOPLE_FILE_ERROR(peopleId, format, ...) LOG_SEND_PEOPLE_FILE_ERROR_EX(LogSenderManager::instance().logName().c_str(), peopleId, format, ##__VA_ARGS__)
#define LOG_SEND_PEOPLE_FILE_FATAL(peopleId, format, ...) LOG_SEND_PEOPLE_FILE_FATAL_EX(LogSenderManager::instance().logName().c_str(), peopleId, format, ##__VA_ARGS__)

#define LOG_SEND_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(0, LogPackage::LOG_INFO, true, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_DEBUG_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(0, LogPackage::LOG_DEBUG, true, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_INFO_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(0, LogPackage::LOG_INFO, true, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_WARNING_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(0, LogPackage::LOG_WARNING, true, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_ERROR_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(0, LogPackage::LOG_ERROR, true, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_FATAL_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(0, LogPackage::LOG_FATAL, true, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}

#define LOG_SEND_PEOPLE_EX(logName, peopleId, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(peopleId, LogPackage::LOG_INFO, true, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_PEOPLE_DEBUG_EX(logName, peopleId, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(peopleId, LogPackage::LOG_DEBUG, true, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_PEOPLE_INFO_EX(logName, peopleId, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(peopleId, LogPackage::LOG_INFO, true, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_PEOPLE_WARNING_EX(logName, peopleId, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(peopleId, LogPackage::LOG_WARNING, true, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_PEOPLE_ERROR_EX(logName, peopleId, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(peopleId, LogPackage::LOG_ERROR, true, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_PEOPLE_FATAL_EX(logName, peopleId, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(peopleId, LogPackage::LOG_FATAL, true, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}

#define LOG_SEND_LOCAL_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(0, LogPackage::LOG_INFO, false, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_LOCAL_DEBUG_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(0, LogPackage::LOG_DEBUG, false, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_LOCAL_INFO_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(0, LogPackage::LOG_INFO, false, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_LOCAL_WARNING_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(0, LogPackage::LOG_WARNING, false, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_LOCAL_ERROR_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(0, LogPackage::LOG_ERROR, false, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_LOCAL_FATAL_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(0, LogPackage::LOG_FATAL, false, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}

#define LOG_SEND_PEOPLE_LOCAL_EX(logName, peopleId, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(peopleId, LogPackage::LOG_INFO, false, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_PEOPLE_LOCAL_DEBUG_EX(logName, peopleId, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(peopleId, LogPackage::LOG_DEBUG, false, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_PEOPLE_LOCAL_INFO_EX(logName, peopleId, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(peopleId, LogPackage::LOG_INFO, false, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_PEOPLE_LOCAL_WARNING_EX(logName, peopleId, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(peopleId, LogPackage::LOG_WARNING, false, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_PEOPLE_LOCAL_ERROR_EX(logName, peopleId, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(peopleId, LogPackage::LOG_ERROR, false, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_PEOPLE_LOCAL_FATAL_EX(logName, peopleId, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(peopleId, LogPackage::LOG_FATAL, false, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}

#define LOG_SEND_FILE_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(0, LogPackage::LOG_INFO, false, false, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_FILE_DEBUG_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(0, LogPackage::LOG_DEBUG, false, false, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_FILE_INFO_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(0, LogPackage::LOG_INFO, false, false, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_FILE_WARNING_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(0, LogPackage::LOG_WARNING, false, false, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_FILE_ERROR_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(0, LogPackage::LOG_ERROR, false, false, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_FILE_FATAL_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(0, LogPackage::LOG_FATAL, false, false, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}

#define LOG_SEND_PEOPLE_FILE_EX(logName, peopleId, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(peopleId, LogPackage::LOG_INFO, false, false, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_PEOPLE_FILE_DEBUG_EX(logName, peopleId, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(peopleId, LogPackage::LOG_DEBUG, false, false, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_PEOPLE_FILE_INFO_EX(logName, peopleId, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(peopleId, LogPackage::LOG_INFO, false, false, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_PEOPLE_FILE_WARNING_EX(logName, peopleId, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(peopleId, LogPackage::LOG_WARNING, false, false, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_PEOPLE_FILE_ERROR_EX(logName, peopleId, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(peopleId, LogPackage::LOG_ERROR, false, false, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_PEOPLE_FILE_FATAL_EX(logName, peopleId, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(peopleId, LogPackage::LOG_FATAL, false, false, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}

typedef LogSenderInterface*(*LogSenderInstance)();

class LogSenderManager
{
protected:
	/** 构造函数
	*/
	LogSenderManager():
		m_interface(nullptr)
	{

	}

	/** 析构函数
	*/
	~LogSenderManager()
	{
#ifdef _MSC_VER
		::FreeLibrary(m_dllHinstance);
#elif __unix__
		::dlclose(m_dllHinstance);
#endif
	}


public:
	/** 单一实例
	@return 返回单一实例
	*/
	static LogSenderManager& instance()
	{
		static LogSenderManager s_logSenderManager;
		return s_logSenderManager;
	}

public:
	void setLogName(const std::string& logName)
	{
		m_logName = logName;
	}

	std::string logName()
	{
		return m_logName;
	}

	LogSenderInterface* getInterface()
	{
		if (m_interface != nullptr)
		{
			return m_interface;
		}

		if (m_dllHinstance == nullptr)
		{
			char szFilePath[1024] = {};
#ifdef _MSC_VER
			::GetModuleFileNameA(NULL, szFilePath, 1024);
#elif __unix__
			int ignore __attribute__((unused)) = ::readlink("/proc/self/exe", szFilePath, 1024);
#endif
			std::string path = szFilePath;
			int32_t left = (int32_t)path.find_last_of("/\\");
			std::string dllName = "LogSender";
#ifdef _MSC_VER
#ifdef _WIN64
			dllName.append("_64.dll");
#else
			dllName.append(".dll");
#endif
#else
#ifdef __x86_64__
			dllName.append("_64.so");
#elif __aarch64__
			dllName.append("_arm64.so");
#elif __i386__
			dllName.append(".so");
#endif
			dllName = "lib" + dllName;
#endif
#ifdef _MSC_VER
			std::string exeDllPath = path.substr(0, left + 1) + dllName;
			m_dllHinstance = ::LoadLibraryExA(exeDllPath.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
#elif __unix__
			std::string exeDllPath = path.substr(0, left + 1) + dllName;
			m_dllHinstance = ::dlopen(exeDllPath.c_str(), RTLD_LAZY);
#endif
			if (m_dllHinstance == nullptr)
			{
#ifdef _MSC_VER
				m_dllHinstance = ::LoadLibraryExA(("D:/xueyelingshuang/common/LogSender/" + dllName).c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
#elif __unix__
				m_dllHinstance = ::dlopen(("/home/xylsxyls/xueyelingshuang/common/LogSender/" + dllName).c_str(), RTLD_LAZY);
#endif
				if (m_dllHinstance == nullptr)
				{
#ifdef _MSC_VER
					m_dllHinstance = ::LoadLibraryExA(("D:/LogSender/" + dllName).c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
#elif __unix__
					m_dllHinstance = ::dlopen(("/tmp/LogSender/" + dllName).c_str(), RTLD_LAZY);
#endif
				}
			}
		}

		if (m_dllHinstance == nullptr)
		{
			return nullptr;
		}

		if (m_logInstance == nullptr)
		{
#ifdef _MSC_VER
			m_logInstance = (LogSenderInstance)::GetProcAddress(m_dllHinstance, "logInstance");
#elif __unix__
			m_logInstance = (LogSenderInstance)::dlsym(m_dllHinstance, "logInstance");
#endif
		}
		
		if (m_logInstance == nullptr)
		{
			return nullptr;
		}
		m_interface = m_logInstance();
		return m_interface;
	}

protected:
	std::string m_logName;
	LogSenderInterface* m_interface;
#ifdef _MSC_VER
	HINSTANCE m_dllHinstance;
#elif __unix__
	void* m_dllHinstance;
#endif
	LogSenderInstance m_logInstance;
};
