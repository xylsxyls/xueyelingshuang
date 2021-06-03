#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class AttackTask : public CTask
{
public:
	AttackTask();

public:
	void DoTask();

	void StopTask();

	void Sleep(int32_t time);

private:
	std::atomic<bool> m_exit;
};