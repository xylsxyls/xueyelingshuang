#pragma once
#ifdef _WIN32
#define _LogSenderAPI
#endif
#include "LogSender.h"
#ifdef __unix__
#include <unistd.h>
#include <dlfcn.h>
#endif

#define LOG_SEND(format, ...) LOG_SEND_EX("", format, ##__VA_ARGS__)
#define LOG_SEND_LOCAL(logName, format, ...) LOG_SEND_LOCAL_EX("", format, ##__VA_ARGS__)
#define LOG_SEND_DEBUG(logName, format, ...) LOG_SEND_DEBUG_EX("", format, ##__VA_ARGS__)
#define LOG_SEND_ONLY_INFO(logName, format, ...) LOG_SEND_ONLY_INFO_EX("", format, ##__VA_ARGS__)
#define LOG_SEND_INFO(logName, format, ...) LOG_SEND_INFO_EX("", format, ##__VA_ARGS__)
#define LOG_SEND_WARNING(logName, format, ...) LOG_SEND_WARNING_EX("", format, ##__VA_ARGS__)
#define LOG_SEND_ERROR(logName, format, ...) LOG_SEND_ERROR_EX("", format, ##__VA_ARGS__)
#define LOG_SEND_FATAL(logName, format, ...) LOG_SEND_FATAL_EX("", format, ##__VA_ARGS__)

#define LOG_SEND_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(LogPackage::LOG_INFO, true, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_LOCAL_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(LogPackage::LOG_INFO, false, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_DEBUG_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(LogPackage::LOG_DEBUG, false, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_ONLY_INFO_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(LogPackage::LOG_INFO, false, false, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_INFO_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(LogPackage::LOG_INFO, false, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_WARNING_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(LogPackage::LOG_WARNING, true, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_ERROR_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(LogPackage::LOG_ERROR, true, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}
#define LOG_SEND_FATAL_EX(logName, format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(LogPackage::LOG_FATAL, true, true, true, __FILE__, __FUNCTION__, logName), format, ##__VA_ARGS__);}

typedef LogSenderInterface&(*LogSenderInstance)();

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
			::readlink("/proc/self/exe", szFilePath, 1024);
#endif
			std::string path = szFilePath;
			int32_t left = (int32_t)path.find_last_of("/\\");
#ifdef _MSC_VER
			std::string exeDllPath = path.substr(0, left + 1) + "LogSender.dll";
			m_dllHinstance = ::LoadLibraryExA(exeDllPath.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
#elif __unix__
			std::string exeDllPath = path.substr(0, left + 1) + "libLogSender.so";
			m_dllHinstance = ::dlopen(exeDllPath.c_str(), RTLD_LAZY);
#endif
			if (m_dllHinstance == nullptr)
			{
#ifdef _MSC_VER
				m_dllHinstance = ::LoadLibraryExA("D:/xueyelingshuang/common/LogSender/LogSender.dll", 0, LOAD_WITH_ALTERED_SEARCH_PATH);
#elif __unix__
				m_dllHinstance = ::dlopen("/home/xylsxyls/xueyelingshuang/common/LogSender/libLogSender.so", RTLD_LAZY);
#endif
				if (m_dllHinstance == nullptr)
				{
#ifdef _MSC_VER
					m_dllHinstance = ::LoadLibraryExA("D:/LogSender/LogSender.dll", 0, LOAD_WITH_ALTERED_SEARCH_PATH);
#elif __unix__
					m_dllHinstance = ::dlopen("/tmp/LogSender/libLogSender.so", RTLD_LAZY);
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
		m_interface = &(m_logInstance());
		return m_interface;
	}

protected:
	LogSenderInterface* m_interface;
#ifdef _MSC_VER
	HINSTANCE m_dllHinstance;
#elif __unix__
	void* m_dllHinstance;
#endif
	LogSenderInstance m_logInstance;
};