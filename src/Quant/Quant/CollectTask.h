#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"

class CollectTask : public CTask
{
public:
	CollectTask();

public:
	void DoTask();

	void StopTask();

	void setParam(int32_t count);

	void Sleep(int32_t time);

private:
	Semaphore m_sleep;
	int32_t m_count;
	std::atomic<bool> m_exit;
};