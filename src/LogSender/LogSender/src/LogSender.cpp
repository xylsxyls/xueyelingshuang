#include "LogSender.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "SharedMemory/SharedMemoryAPI.h"

LogSender::LogSender():
m_message(nullptr),
m_logTestPid(nullptr)
{
	m_computerName = CSystem::getComputerName();
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
	(*m_message)[LOG_INT_DATE_TIME] = time;
	(*m_message)[LOG_LEVEL] = package.m_logLevel;
	(*m_message)[LOG_IS_SEND_NET] = (int32_t)package.m_isSendNet;
	(*m_message)[LOG_IS_SEND_SCREEN] = (int32_t)package.m_isSendScreen;
	(*m_message)[LOG_IS_WRITE_LOG] = (int32_t)package.m_isWriteLog;
	(*m_message)[LOG_THREAD_ID] = (int32_t)CSystem::SystemThreadId();

	(*m_message)[LOG_FILE_NAME] = package.m_fileName;
	(*m_message)[LOG_FUN_NAME] = package.m_funName;
	(*m_message)[LOG_BUFFER] = str;
	
	std::string strMessage;
	(*m_message).toString(strMessage);
	send(strMessage.c_str(), strMessage.length());
}

void LogSender::send(const char* buffer, int32_t length)
{
	ProcessWork::instance().send(buffer, length, SharedMemory::readPid("LogTest1.0.exe", m_logTestPid), CorrespondParam::PROTO_MESSAGE);
}

void LogSender::uninit()
{
	ProcessWork::instance().uninit();
}