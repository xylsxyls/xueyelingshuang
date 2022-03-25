#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"

class SleepBaseTask : public CTask
{
public:
	SleepBaseTask();

public:
	void StopTask();

	void sleep(int32_t time);

protected:
	Semaphore m_sleep;
	std::atomic<bool> m_exit;
};