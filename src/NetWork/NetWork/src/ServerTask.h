#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class NetServer;

class ServerTask : public CTask
{
public:
	ServerTask();

public:
	void DoTask();

	void setParam(int32_t port, NetServer* netServer);

private:
	int32_t m_port;
	NetServer* m_netServer;
};