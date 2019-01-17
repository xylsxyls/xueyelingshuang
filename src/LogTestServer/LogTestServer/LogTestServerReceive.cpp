#include "LogTestServerReceive.h"
#include "NetSender/NetSenderAPI.h"
#include "NetLineManager.h"
#include "Cini/CiniAPI.h"
#include "CStringManager/CStringManagerAPI.h"

void LogTestServerReceive::receive(char* buffer, int32_t length, int32_t sendPid, int32_t protocolId)
{
	ProtoMessage message;
	message.from(std::string(buffer, length));
	switch (protocolId)
	{
	case ProcessWork::INIT:
	{
		printf("INIT, buffer = %s, length = %d\n", buffer, length);
		HandleInitMessage(message);
		break;
	}
	case ProcessWork::PROTO_MESSAGE:
	{
		printf("PROTO_MESSAGE, buffer = %s, length = %d\n", buffer, length);
		HandleMessage(message);
		break;
	}
	case ProcessWork::JSON:
	{
		break;
	}
	case ProcessWork::XML:
	{
		break;
	}
	default:
		break;
	}
}

void LogTestServerReceive::HandleInitMessage(ProtoMessage& message)
{
	auto messageMap = message.getMap();
	NetLineManager::instance().addConnect(messageMap["loginName"].toString(), (uv_tcp_t*)(int32_t)messageMap["ClientPtr"]);
	printf("init send to netserver\n");
	NetSender::instance().send(message.toString().c_str(), message.toString().length(), ProcessWork::INIT, true);
}

void LogTestServerReceive::HandleMessage(ProtoMessage& message)
{
	Cini ini("config.ini");
	std::string collectComputerName = ini.ReadIni("CollectComputerName");
	std::vector<std::string> vecComputerName = CStringManager::split(collectComputerName, ",");
	int32_t index = -1;
	while (index++ != vecComputerName.size() - 1)
	{
		auto& computerName = vecComputerName[index];
		uv_tcp_t* clientPtr = NetLineManager::instance().findConnect(computerName);
		message["ClientPtr"] = (int32_t)clientPtr;
		printf("send to netserver\n");
		NetSender::instance().send(message.toString().c_str(), message.toString().length(), ProcessWork::PROTO_MESSAGE, true);
	}
}