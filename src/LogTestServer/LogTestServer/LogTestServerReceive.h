#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"

class LogTestServerReceive : public ReceiveCallback
{
public:
	LogTestServerReceive();

public:
	virtual void receive(char* buffer, int32_t length, int32_t sendPid, CorrespondParam::ProtocolId protocolId);

	void HandleMessage(ProtoMessage& message, int32_t clientId);

	int32_t getClientId(const char* buffer, int32_t length);

	std::string set4ClientId(int32_t clientId);

private:
	std::string m_iniPath;
};