#pragma once
#include "LogSenderMacro.h"
#include <stdint.h>
#include "LogPackage.h"

#define LOG_SEND_ALL(format, ...)  LogSender::instance().logSend(LogPackage(LogPackage::LOG_DEBUG, __FILE__, __FUNCTION__, true, true, true), format, ##__VA_ARGS__)
#define LOG_SEND_DEBUG(format, ...) LogSender::instance().logSend(LogPackage(LogPackage::LOG_DEBUG, __FILE__, __FUNCTION__, false, true, true), format, ##__VA_ARGS__)
#define LOG_SEND_INFO(format, ...) LogSender::instance().logSend(LogPackage(LogPackage::LOG_INFO, __FILE__, __FUNCTION__, false, true, true), format, ##__VA_ARGS__)
#define LOG_SEND_WARNING(format, ...) LogSender::instance().logSend(LogPackage(LogPackage::LOG_WARNING, __FILE__, __FUNCTION__, true, true, true), format, ##__VA_ARGS__)
#define LOG_SEND_ERROR(format, ...) LogSender::instance().logSend(LogPackage(LogPackage::LOG_ERROR, __FILE__, __FUNCTION__, true, true, true), format, ##__VA_ARGS__)
#define LOG_SEND_FATAL(format, ...) LogSender::instance().logSend(LogPackage(LogPackage::LOG_FATAL, __FILE__, __FUNCTION__, true, true, true), format, ##__VA_ARGS__)

class LogSenderAPI LogSender
{
public:
	
protected:
	LogSender();

public:
	static LogSender& instance();

public:
	void logSend(const LogPackage& package, const char* format, ...);

	void uninit();

protected:
	void send(const char* buffer, int32_t length);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_computerName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};