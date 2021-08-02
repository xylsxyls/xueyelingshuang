#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <string>

class NetClient;

class ClientTask : public CTask
{
public:
	ClientTask();

public:
	void DoTask();

	void setParam(const std::string& ip, int32_t port, NetClient* netClient);

private:
	std::string m_ip;
	int32_t m_port;
	NetClient* m_netClient;
};