#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"

class Cwq2Task : public CTask
{
public:
	Cwq2Task();

public:
	void DoTask();

	void StopTask();

	bool Sleep(int32_t sleepTime);

	void KeyPressE();

	void KeyPressF();

	void ClickHero(char heroNum);

private:
	Semaphore m_sleep;
	std::atomic<bool> m_exit;
};