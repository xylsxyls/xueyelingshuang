#pragma once
#include "ProcessClient/ProcessClientAPI.h"

class LogTestServerReceive : public ReceiveCallback
{
public:
	virtual void receive(char* buffer, int32_t length, int32_t sendPid, int32_t protocolId);
};