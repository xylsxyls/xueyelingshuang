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

void NetSender::send(const char* buffer, int32_t length, int32_t protocolId, bool isServer)
{
	ProcessClient::instance().send(buffer, length, isServer ? "NetServerManager.exe" : "NetClientManager.exe", protocolId);
}
