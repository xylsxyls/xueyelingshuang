#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"

class NetServer;

class HeadTask : public CTask
{
public:
	HeadTask();

public:
	void DoTask();

	void StopTask();

	void setParam(int32_t time, NetServer* netServer);

private:
	int32_t m_time;
	NetServer* m_netServer;
	Semaphore m_sleep;
	std::atomic<bool> m_exit;
};