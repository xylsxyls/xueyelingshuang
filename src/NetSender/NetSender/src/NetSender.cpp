#include "NetSender.h"
#include "CSystem/CSystemAPI.h"

NetSender::NetSender()
{

}

NetSender& NetSender::instance()
{
	static NetSender s_netSender;
	return s_netSender;
}

void NetSender::init(ProtoMessage& message, CorrespondParam::ProtocolId protocolId, bool isServer /*= false*/)
{
	if (!isServer)
	{
		message[CLIENT_PID] = (int32_t)CSystem::currentProcessPid();
	}
	std::string strMessage = message.toString();
	ProcessWork::instance().send(isServer ? "NetServerManager1.0" : "NetClientManager1.0",
		strMessage.c_str(),
		strMessage.length(),
		protocolId);
}

void NetSender::send(ProtoMessage& message, bool isServer)
{
	if (!isServer)
	{
		message[CLIENT_PID] = (int32_t)CSystem::currentProcessPid();
	}
	std::string strMessage = message.toString();
	ProcessWork::instance().send(isServer ? "NetServerManager1.0" : "NetClientManager1.0",
		strMessage.c_str(),
		strMessage.length(),
		CorrespondParam::ProtocolId::PROTO_MESSAGE);
}

void NetSender::initPostThread()
{
	ProcessWork::instance().initPostThread();
}

void NetSender::initReceive(ProcessReceiveCallback* callback, int32_t receiveSize, int32_t areaCount)
{
	ProcessWork::instance().initReceive(callback, receiveSize, areaCount);
}

void NetSender::uninitReceive()
{
	ProcessWork::instance().uninitReceive();
}

void NetSender::uninitPostThread()
{
	ProcessWork::instance().uninitPostThread();
}

void NetSender::post(ProtoMessage& message, bool isServer)
{
	if (!isServer)
	{
		message[CLIENT_PID] = (int32_t)CSystem::currentProcessPid();
	}
	std::string strMessage = message.toString();
	ProcessWork::instance().post(isServer ? "NetServerManager1.0" : "NetClientManager1.0",
		strMessage.c_str(),
		strMessage.length(),
		CorrespondParam::ProtocolId::PROTO_MESSAGE);
}