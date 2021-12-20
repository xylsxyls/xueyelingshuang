#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"

class Cwq2nofTask : public CTask
{
public:
	Cwq2nofTask();

public:
	void DoTask();

	void StopTask();

	bool Sleep(int32_t sleepTime);

	void KeyPressE();

	void KeyPressF();

private:
	Semaphore m_sleep;
	std::atomic<bool> m_exit;
};