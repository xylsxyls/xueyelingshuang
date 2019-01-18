#include "NetSender.h"
#include "ProcessWork/ProcessWorkAPI.h"

NetSender::NetSender()
{

}

NetSender& NetSender::instance()
{
	static NetSender netSender;
	return netSender;
}

void NetSender::init(bool isServer)
{
	ProcessWork::instance().send(nullptr, 0, isServer ? "NetServerManager1.0.exe" : "NetClientManager1.0.exe", ProcessWork::INIT);
}

void NetSender::send(const char* buffer, int32_t length, int32_t protocolId, bool isServer)
{
	ProcessWork::instance().send(buffer, length, isServer ? "NetServerManager1.0.exe" : "NetClientManager1.0.exe", protocolId);
}