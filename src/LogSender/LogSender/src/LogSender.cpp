#include "LogSender.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "../../../NetServerManager/NetServerManager/ProtocolId.h"
#include "ProtoMessage/ProtoMessageAPI.h"

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
	ProtoMessage message;
	message["buffer"] = buffer;
	ProcessWork::instance().send(message.toString().c_str(), message.toString().length(), "LogTest1.0.exe", ProtocolId::PROTO_MESSAGE);
}

void LogSender::uninit()
{
	ProcessWork::instance().uninit();
}