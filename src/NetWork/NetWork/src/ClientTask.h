#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <string>
#include "Semaphore/SemaphoreAPI.h"

class NetClient;

class ClientTask : public CTask
{
public:
	ClientTask();

public:
	void DoTask();
	
	void StopTask();

	void setParam(int32_t waitTime, const std::string& ip, int32_t port, NetClient* netClient);

private:
	int32_t m_waitTime;
	Semaphore m_wait;
	std::string m_ip;
	int32_t m_port;
	NetClient* m_netClient;
};