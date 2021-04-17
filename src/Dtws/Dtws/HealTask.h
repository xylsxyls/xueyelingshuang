#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>

class HealTask : public CTask
{
public:
	HealTask();

public:
	void DoTask();

	void StopTask();

	void Sleep(int32_t time);

private:
	std::atomic<bool> m_exit;
};