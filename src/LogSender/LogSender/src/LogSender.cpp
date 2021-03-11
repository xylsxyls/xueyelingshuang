#include "LogSender.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CSystem/CSystemAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "SharedMemory/SharedMemoryAPI.h"

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

void LogSender::logSend(const LogPackage& package, const char* format, ...)
{
	SharedMemory sharedMemory("ProcessArea_LogTest1.0");
	if (sharedMemory.writeWithoutLock() == nullptr)
	{
		return;
	}

	std::string str;
	va_list args = nullptr;
	va_start(args, format);
	int32_t size = ::_vscprintf(format, args);
	//?resize�����string����Զ���������\0��resize(5)���ܳ�6
	str.resize(size);
	//?����������㹻�ڴ棬���ȱ����1����������
	::vsprintf_s(&str[0], size + 1, format, args);
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

		(*m_message)[LOG_FILE_NAME] = package.m_fileName;
		(*m_message)[LOG_FUN_NAME] = package.m_funName;
		(*m_message)[LOG_BUFFER] = str;

		(*m_message).toString(strMessage);
	}
	
	send(strMessage.c_str(), strMessage.length());
}

void LogSender::send(const char* buffer, int32_t length)
{
	ProcessWork::instance().send("LogTest1.0", buffer, length, CorrespondParam::PROTO_MESSAGE);
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