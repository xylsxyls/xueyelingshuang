#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"

class LogTestServerReceive : public ReceiveCallback
{
public:
	virtual void receive(char* buffer, int32_t length, int32_t sendPid, int32_t protocolId);

	void HandleInitMessage(ProtoMessage& message);

	void HandleMessage(ProtoMessage& message);
};