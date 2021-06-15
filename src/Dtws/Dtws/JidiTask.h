#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"

class JidiTask : public CTask
{
public:
	JidiTask();

public:
	void DoTask();

	void StopTask();

	void setParam(int32_t clientIndex);

	void Sleep(int32_t time);

private:
	Semaphore m_sleep;
	int32_t m_clientIndex;
	std::atomic<bool> m_exit;
};