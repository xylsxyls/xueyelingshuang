#include "NetSender.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "SharedMemory/SharedMemoryAPI.h"

NetSender::NetSender():
m_clientManagerPid(nullptr),
m_serverManagerPid(nullptr)
{

}

NetSender& NetSender::instance()
{
	static NetSender s_netSender;
	return s_netSender;
}

void NetSender::init(const char* buffer, int32_t length, bool isServer)
{
	send(buffer, length, isServer ? CorrespondParam::SERVER_INIT : CorrespondParam::CLIENT_INIT, isServer);
}

void NetSender::send(const char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId, bool isServer)
{
	ProcessWork::instance().send(buffer, length, isServer ? SharedMemory::readPid("NetServerManager1.0.exe", m_serverManagerPid) : SharedMemory::readPid("NetClientManager1.0.exe", m_clientManagerPid), protocolId);
}