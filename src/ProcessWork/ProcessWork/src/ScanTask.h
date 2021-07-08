#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"

class SharedMemory;

class ScanTask : public CTask
{
public:
	ScanTask();

public:
	void DoTask();

	void StopTask();

	void setParam(SharedMemory* areaAssign);

	void Sleep(int32_t timeout);

private:
	SharedMemory* m_areaAssign;
	Semaphore m_sleep;
	std::atomic<bool> m_exit;
};