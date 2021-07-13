#include "LogSender.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CSystem/CSystemAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "SharedMemory/SharedMemoryAPI.h"
#ifdef __unix__
#include <stdarg.h>
#endif
#include "LogTestMessage.pb.h"

LogSenderAPI LogSenderInterface& logInstance()
{
	return LogSender::instance();
}

LogSender::LogSender()
{
	m_processName = CSystem::GetCurrentExeFullName();
	m_loginName = CSystem::getComputerName();
}

LogSender& LogSender::instance()
{
	static LogSender logSender;
	return logSender;
}

void LogSender::logTestUniqueOpen()
{
	if (logTestExist())
	{
		return;
	}
	logTestOpen();
}

void LogSender::logTestOpen()
{
	std::string path = CSystem::GetCurrentDllPath() + "LogTest" + LOGTEST_CLIENT_VERSION;
#ifdef _MSC_VER
	path.append(".exe");
	::ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_HIDE);
#elif __unix__
	system(("setsid " + path + " &").c_str());
#endif
	while (!logTestExist())
	{
		CSystem::Sleep(1);
	}
}

void LogSender::logSend(const LogPackage& package, const char* format, ...)
{
	std::string str;
	va_list args;
	va_start(args, format);
#ifdef _WIN32
	int size = _vscprintf(format, args);
#elif __unix__
	va_list argcopy;
	va_copy(argcopy, args);
	int size = vsnprintf(nullptr, 0, format, argcopy);
#endif
	//?resize分配后string类会自动在最后分配\0，resize(5)则总长6
	str.resize(size);
	if (size != 0)
	{
#ifdef _WIN32
		//?即便分配了足够内存，长度必须加1，否则会崩溃
		vsprintf_s(&str[0], size + 1, format, args);
#elif __unix__
		vsnprintf(&str[0], size + 1, format, args);
#endif
	}
	va_end(args);

	uint64_t time = 0;
	uint64_t* timePtr = &time;
	IntDateTime currentTime;
	*((int32_t*)timePtr) = currentTime.getDate();
	*((int32_t*)timePtr + 1) = currentTime.getTime();

	bool isSend = false;
	bool isLocal = false;
	logtest::LogTestMessage message;
	message.set_logintdatetime(time);
	message.set_loglevel(package.m_logLevel);
	message.set_logthreadid(CSystem::SystemThreadId());
	message.set_logname(package.m_logName);
	message.set_logfilename(package.m_fileName);
	message.set_logfunname(package.m_funName);
	if (package.m_isWriteLog)
	{
		message.set_logprocessname(m_processName);
	}
	if (package.m_isSendNet)
	{
		message.set_logprocessname(m_processName);
		message.set_logloginname(m_loginName);
		isSend = true;
	}
	message.set_logbuffer(str);
	std::string strMessage;
	message.SerializeToString(&strMessage);

	if (package.m_isSendScreen)
	{
		isLocal = true;
	}
	
	send(strMessage, isSend ? LOGTEST_SEND_MESSAGE : isLocal ? LOGTEST_LOCAL_MESSAGE : LOGTEST_ONLY_MESSAGE);
}

void LogSender::logTestClose()
{
	SharedMemory sharedMemory("ProcessArea_LogTest" + LOGTEST_CLIENT_VERSION);
	if (sharedMemory.writeWithoutLock() == nullptr)
	{
		return;
	}

	ProtoMessage message;
	message[LOG_CLOSE] = (int32_t)true;
	std::string strMessage;
	message.toString(strMessage);

	send(strMessage, LOGTEST_MESSAGE);

	while (logTestExist())
	{
		CSystem::Sleep(1);
	}
}

void LogSender::set(bool dealLog)
{
	SharedMemory sharedMemory("ProcessArea_LogTest" + LOGTEST_CLIENT_VERSION);
	if (sharedMemory.writeWithoutLock() == nullptr)
	{
		return;
	}

	ProtoMessage message;
	message[LOG_SET] = (int32_t)true;
	message[LOG_SET_DEAL_LOG] = (int32_t)dealLog;
	std::string strMessage;
	message.toString(strMessage);

	send(strMessage, LOGTEST_MESSAGE);
}

bool LogSender::logTestExist()
{
	SharedMemory sharedMemory("ProcessArea_LogTest" + LOGTEST_CLIENT_VERSION);
	void* logTestMemory = sharedMemory.writeWithoutLock();
	if (logTestMemory == nullptr)
	{
		return false;
	}

	int32_t logTestPid = *(int32_t*)logTestMemory;
	std::string logTestName = "LogTest" + LOGTEST_CLIENT_VERSION;
#ifdef _MSC_VER
	logTestName.append(".exe");
#endif
	return (CSystem::processFirstPid(logTestName) == logTestPid);
}

void LogSender::send(const std::string& message, int32_t type)
{
	ProcessWork::instance().send("LogTest" + LOGTEST_CLIENT_VERSION, message, (MessageType)type);
}