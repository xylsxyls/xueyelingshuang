#pragma once
#include "ProcessWork/ProcessWorkAPI.h"

class NetClient;

class ProcessReceive : public ReceiveCallback
{
public:
	ProcessReceive();

public:
	virtual void receive(char* buffer, int32_t length, int32_t sendPid, int32_t protocalId);

	void setNetClient(NetClient* netClient);

protected:
	NetClient* m_netClient;
};