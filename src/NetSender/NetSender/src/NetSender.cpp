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

void NetSender::init(const char* buffer, int32_t length, bool isServer)
{
	ProcessWork::instance().send(buffer, length, isServer ? "NetServerManager1.0.exe" : "NetClientManager1.0.exe", isServer ? CorrespondParam::SERVER_INIT : CorrespondParam::CLIENT_INIT);
}

void NetSender::send(const char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId, bool isServer)
{
	ProcessWork::instance().send(buffer, length, isServer ? "NetServerManager1.0.exe" : "NetClientManager1.0.exe", protocolId);
}