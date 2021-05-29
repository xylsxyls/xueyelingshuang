#include "LogSender.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CSystem/CSystemAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "SharedMemory/SharedMemoryAPI.h"
#ifdef __unix__
#include <stdarg.h>
#endif

LogSenderAPI LogSenderInterface& logInstance()
{
	return LogSender::instance();
}

LogSender::LogSender():
m_message(nullptr)
{
	m_message = new ProtoMessage;
}

LogSender::~LogSender()
{
	if (m_message != nullptr)
	{
		delete m_message;
		m_message = nullptr;
	}
}

LogSender& LogSender::instance()
{
	static LogSender logSender;
	return logSender;
}

void LogSender::logTestOpen()
{
	std::string path = CSystem::GetCurrentDllPath() + "LogTest" + LOGTEST_CLIENT_VERSION;
#ifdef _MSC_VER
	path.append(".exe");
#endif
	::ShellExecuteA(NULL, "open", path.c_str(), NULL, NULL, SW_HIDE);
	while (!logTestExist())
	{
		CSystem::Sleep(1);
	}
}

void LogSender::logSend(const LogPackage& package, const char* format, ...)
{
	SharedMemory sharedMemory("ProcessArea_LogTest" + LOGTEST_CLIENT_VERSION);
	if (sharedMemory.writeWithoutLock() == nullptr)
	{
		return;
	}

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

	std::string strMessage;
	{
		std::unique_lock<std::mutex> lock(m_messageMutex);
		(*m_message)[LOG_INT_DATE_TIME] = time;
		(*m_message)[LOG_LEVEL] = package.m_logLevel;
		(*m_message)[LOG_IS_SEND_NET] = (int32_t)package.m_isSendNet;
		(*m_message)[LOG_IS_SEND_SCREEN] = (int32_t)package.m_isSendScreen;
		(*m_message)[LOG_IS_WRITE_LOG] = (int32_t)package.m_isWriteLog;
		(*m_message)[LOG_THREAD_ID] = (int32_t)CSystem::SystemThreadId();

		(*m_message)[LOG_NAME] = package.m_logName;
		(*m_message)[LOG_FILE_NAME] = package.m_fileName;
		(*m_message)[LOG_FUN_NAME] = package.m_funName;
		(*m_message)[LOG_BUFFER] = str;

		(*m_message).toString(strMessage);
	}
	
	send(strMessage.c_str(), (int32_t)strMessage.length());
}

void LogSender::logTestClose()
{
	SharedMemory sharedMemory("ProcessArea_LogTest" + LOGTEST_CLIENT_VERSION);
	if (sharedMemory.writeWithoutLock() == nullptr)
	{
		return;
	}

	std::string strMessage;
	{
		std::unique_lock<std::mutex> lock(m_messageMutex);
		m_message->clear();
		(*m_message)[LOG_CLOSE] = (int32_t)true;
		(*m_message).toString(strMessage);
	}

	send(strMessage.c_str(), (int32_t)strMessage.length());
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

void LogSender::send(const char* buffer, int32_t length)
{
	ProcessWork::instance().send("LogTest" + LOGTEST_CLIENT_VERSION, buffer, length, LOGTEST_MESSAGE);
}

//int32_t main(int32_t argc, char** argv)
//{
//	int32_t count = 0;
//	if (argc == 1)
//	{
//		count = 1;
//	}
//	else
//	{
//		count = atoi(argv[1]);
//	}
//	while (count-- != 0)
//	{
//		LOG_SEND("123456");
//	}
//	return 0;
//}