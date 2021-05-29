#pragma once
#ifdef _WIN32
#define _LogSenderAPI
#endif
#include "LogSender.h"
#ifdef __unix__
#include <unistd.h>
#include <dlfcn.h>
#endif

#define LOG_TEST_EXIST LogSenderManager::instance().getInterface()->logTestExist
#define LOG_TEST_OPEN LogSenderManager::instance().getInterface()->logTestOpen
#define LOG_TEST_CLOSE LogSenderManager::instance().getInterface()->logTestClose

#define LOG_NAME_SET(name) LogSenderManager::instance().setLogName(name)

#define LOG_SEND(format, ...) LOG_SEND_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_LOCAL(logName, format, ...) LOG_SEND_LOCAL_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_DEBUG(logName, format, ...) LOG_SEND_DEBUG_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_ONLY_INFO(logName, format, ...) LOG_SEND_ONLY_INFO_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_INFO(logName, format, ...) LOG_SEND_INFO_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_WARNING(logName, format, ...) LOG_SEND_WARNING_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_ERROR(logName, format, ...) LOG_SEND_ERROR_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)
#define LOG_SEND_FATAL(logName, format, ...) LOG_SEND_FATAL_EX(LogSenderManager::instance().logName().c_str(), format, ##__VA_ARGS__)

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
	/** ���캯��
	*/
	LogSenderManager():
		m_interface(nullptr)
	{

	}

	/** ��������
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
	/** ��һʵ��
	@return ���ص�һʵ��
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
			::readlink("/proc/self/exe", szFilePath, 1024);
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
			dllName = "lib" + dllName;
#elif __i386__
			dllName.append(".so");
			dllName = "lib" + dllName;
#endif
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
		m_interface = &(m_logInstance());
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