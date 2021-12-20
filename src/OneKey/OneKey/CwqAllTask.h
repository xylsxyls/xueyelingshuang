#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"

class CwqAllTask : public CTask
{
public:
	CwqAllTask();

public:
	void DoTask();

	void StopTask();

	void LockHero(const std::string& text);

	void ClickHero(char heroNum);

	bool Sleep(int32_t sleepTime);

	void KeyPressE();

	void KeyPressF();

	void setParam(bool isEdit);

private:
	bool m_isEdit;
	Semaphore m_sleep;
	std::atomic<bool> m_exit;
};