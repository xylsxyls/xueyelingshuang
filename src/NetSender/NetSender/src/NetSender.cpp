#include "NetSender.h"
#include "ProcessClient/ProcessClientAPI.h"

NetSender::NetSender()
{

}

NetSender& NetSender::instance()
{
	static NetSender netSender;
	return netSender;
}

void NetSender::send(const char* buffer, int32_t length)
{
	ProcessClient::instance().send(buffer, length, "NetClientManagerd.exe");
}
