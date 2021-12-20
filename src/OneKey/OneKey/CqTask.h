#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>
#include "Semaphore/SemaphoreAPI.h"

class CqTask : public CTask
{
public:
	CqTask();

public:
	void DoTask();

	void StopTask();

	void setParam(int32_t times, int32_t sleepTime);

	bool Sleep(int32_t time);

private:
	int32_t m_times;
	int32_t m_sleepTime;
	Semaphore m_sleep;
	std::atomic<bool> m_exit;
};