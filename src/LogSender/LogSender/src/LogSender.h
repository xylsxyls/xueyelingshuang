#pragma once
#include "LogSenderMacro.h"
#include <stdint.h>
#include <mutex>
#include "LogPackage.h"

class ProtoMessage;

const std::string LOGTEST_CLIENT_VERSION = "1.1";

class LogSenderAPI LogSenderInterface
{
public:
	virtual void logSend(const LogPackage& package, const char* format, ...) = 0;
};

extern "C"
{
	LogSenderAPI LogSenderInterface& logInstance();
}

class LogSenderAPI LogSender : public LogSenderInterface
{
protected:
	LogSender();

	~LogSender();

public:
	static LogSender& instance();

public:
	void logSend(const LogPackage& package, const char* format, ...);

protected:
	void send(const char* buffer, int32_t length);

private:
	ProtoMessage* m_message;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::mutex m_messageMutex;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};