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

	void HandleInitMessage(ProtoMessage& message);

	void HandleMessage(ProtoMessage& message);

private:
	std::string m_iniPath;
};