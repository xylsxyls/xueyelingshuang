#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"

class CSleepTask : public CTask
{
public:
	CSleepTask();

public:
	void DoTask();

	void StopTask();

	bool ReExecute();

	void setParam(int32_t sleepTime);

private:
	Semaphore m_sleep;
	int32_t m_sleepTime;
};