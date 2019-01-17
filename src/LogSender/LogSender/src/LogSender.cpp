#include "LogSender.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CSystem/CSystemAPI.h"

LogSender::LogSender()
{
	m_computerName = CSystem::getComputerName();
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

	ProtoMessage message;
	message["logLevel"] = package.m_logLevel;
	message["fileName"] = package.m_fileName;
	message["funName"] = package.m_funName;
	message["loginName"] = m_computerName;
	message["isSendNet"] = (int32_t)package.m_isSendNet;
	message["isSendScreen"] = (int32_t)package.m_isSendScreen;
	message["isWriteLog"] = (int32_t)package.m_isWriteLog;
	message["buffer"] = str;
	send(message.toString().c_str(), message.toString().length());
}

void LogSender::send(const char* buffer, int32_t length)
{
	ProcessWork::instance().send(buffer, length, "LogTest1.0.exe", ProcessWork::PROTO_MESSAGE);
}

void LogSender::uninit()
{
	ProcessWork::instance().uninit();
}