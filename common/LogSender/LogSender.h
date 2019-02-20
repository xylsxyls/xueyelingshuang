#pragma once
#include "LogSenderMacro.h"
#include <stdint.h>
#include "LogPackage.h"

class ProtoMessage;
class SharedMemory;

class LogSenderAPI LogSenderInterface
{
public:
	virtual void logSend(const LogPackage& package, const char* format, ...) = 0;

	virtual void uninit() = 0;
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

	void uninit();

protected:
	void send(const char* buffer, int32_t length);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_computerName;
	ProtoMessage* m_message;
	SharedMemory* m_logTestPid;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};