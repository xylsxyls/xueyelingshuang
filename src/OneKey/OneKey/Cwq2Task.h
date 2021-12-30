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

	void setParam(uint32_t editIndex);

private:
	uint32_t m_editIndex;
	Semaphore m_sleep;
	std::atomic<bool> m_exit;
};