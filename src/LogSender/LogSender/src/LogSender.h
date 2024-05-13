#pragma once
#include "LogSenderMacro.h"
#include <stdint.h>
#include <mutex>
#include "LogPackage.h"

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