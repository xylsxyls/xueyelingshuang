#include "NetSender.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
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
		message[CLIENT_PID] = CSystem::processFirstPid();
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
		message[CLIENT_PID] = CSystem::processFirstPid();
	}
	std::string strMessage = message.toString();
	ProcessWork::instance().send(isServer ? "NetServerManager1.0" : "NetClientManager1.0",
		strMessage.c_str(),
		strMessage.length(),
		CorrespondParam::ProtocolId::PROTO_MESSAGE);
}