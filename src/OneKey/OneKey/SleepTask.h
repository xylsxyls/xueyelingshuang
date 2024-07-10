#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"

class SleepTask : public CTask
{
public:
	SleepTask();

public:
	void DoTask();

	void setParam(int32_t sleepTime);

private:
	int32_t m_sleepTime;
};