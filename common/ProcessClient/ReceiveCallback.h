#pragma once
#include <stdint.h>

class ReceiveCallback
{
public:
	virtual void receive(char* buffer, int32_t length, int32_t sendPid, int32_t protocolId) = 0;
};