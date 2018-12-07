#include "LogSender.h"
#include "ProcessClient/ProcessClientAPI.h"

LogSender::LogSender()
{

}

LogSender& LogSender::instance()
{
	static LogSender logSender;
	return logSender;
}

void LogSender::send(char* buffer, int32_t length)
{
	ProcessClient::instance().send(buffer, length, "LogTestd.exe");
}
