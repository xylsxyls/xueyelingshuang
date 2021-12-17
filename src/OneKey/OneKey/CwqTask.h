#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"

class CwqTask : public CTask
{
public:
	CwqTask();

public:
	void DoTask();

	void StopTask();

	void setParam(bool isR, bool isF, int32_t key);

	bool Sleep(int32_t sleepTime);

	void KeyPressE();

	void KeyPressF();

	bool LockHero();

	void Lock3();

private:
	bool m_isR;
	bool m_isF;
	int32_t m_key;
	Semaphore m_sleep;
	std::atomic<bool> m_exit;
};