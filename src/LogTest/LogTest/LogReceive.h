#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"

class LogReceive : public ReceiveCallback
{
public:
	LogReceive();

public:
	virtual void receive(char* buffer, int32_t length, int32_t sendPid, CorrespondParam::ProtocolId protocolId);

	void receiveFromNet(char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId);

	void sendToNet(char* buffer, int32_t length, int32_t sendPid, CorrespondParam::ProtocolId protocolId);

	std::string addSendMap(int32_t sendPid);

protected:
	int32_t m_netClientManagerPid;
	std::map<int32_t, std::string> m_sendMap;
};