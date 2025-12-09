#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"

class CqswrTask : public CTask
{
public:
	CqswrTask();

public:
	void DoTask();

	void StopTask();

	bool Sleep(int32_t sleepTime);

	void KeyPressE();

	void KeyPressF();

	void ClickHero(char heroNum);

	void setParam(uint32_t editIndex);

	void V1();

	void V2();

	void V3();

	void V4();

	void V5();

	void Q5();

private:
	uint32_t m_editIndex;
	Semaphore m_sleep;
	std::atomic<bool> m_exit;
};