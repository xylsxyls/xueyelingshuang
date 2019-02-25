#pragma once
#define _LogSenderAPI
#include "LogSender.h"

#define LOG_SEND_ALL(format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(LogPackage::LOG_INFO, true, true, true, __FILE__, __FUNCTION__), format, ##__VA_ARGS__);}
#define LOG_SEND_DEBUG(format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(LogPackage::LOG_DEBUG, false, true, true, __FILE__, __FUNCTION__), format, ##__VA_ARGS__);}
#define LOG_SEND_INFO(format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(LogPackage::LOG_INFO, false, true, true, __FILE__, __FUNCTION__), format, ##__VA_ARGS__);}
#define LOG_SEND_WARNING(format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(LogPackage::LOG_WARNING, true, true, true, __FILE__, __FUNCTION__), format, ##__VA_ARGS__);}
#define LOG_SEND_ERROR(format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(LogPackage::LOG_ERROR, true, true, true, __FILE__, __FUNCTION__), format, ##__VA_ARGS__);}
#define LOG_SEND_FATAL(format, ...) if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->logSend(LogPackage(LogPackage::LOG_FATAL, true, true, true, __FILE__, __FUNCTION__), format, ##__VA_ARGS__);}
#define LOG_SEND_UNINIT() if (LogSenderManager::instance().getInterface() != nullptr){LogSenderManager::instance().getInterface()->uninit();}

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
			::GetModuleFileNameA(NULL, szFilePath, 1024);
			std::string path = szFilePath;
			int32_t left = (int32_t)path.find_last_of("/\\");
			std::string name = path.substr(left + 1, path.length() - left - 1);
			int32_t point = (int32_t)name.find_last_of(".");
			std::string exeDllPath = path.substr(0, left + 1) + "LogSender.dll";
			m_dllHinstance = ::LoadLibraryExA(exeDllPath.c_str(), 0, LOAD_WITH_ALTERED_SEARCH_PATH);
			if (m_dllHinstance == nullptr)
			{
				m_dllHinstance = ::LoadLibraryExA("D:/xueyelingshuang/common/LogSender/LogSender.dll", 0, LOAD_WITH_ALTERED_SEARCH_PATH);
				if (m_dllHinstance == nullptr)
				{
					m_dllHinstance = ::LoadLibraryExA("D:/LogSender/LogSender.dll", 0, LOAD_WITH_ALTERED_SEARCH_PATH);
				}
			}
		}

		if (m_dllHinstance == nullptr)
		{
			return nullptr;
		}

		if (m_logInstance == nullptr)
		{
			m_logInstance = (LogSenderInstance)::GetProcAddress(m_dllHinstance, "logInstance");
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
	HINSTANCE m_dllHinstance;
	LogSenderInstance m_logInstance;
};