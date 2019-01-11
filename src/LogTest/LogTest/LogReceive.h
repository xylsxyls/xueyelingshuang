#pragma once
#include "ProcessWork/ProcessWorkAPI.h"

class LogReceive : public ReceiveCallback
{
public:
	LogReceive();

public:
	virtual void receive(char* buffer, int32_t length, int32_t sendPid, int32_t protocolId);

protected:
	int32_t m_netClientManagerPid;
};