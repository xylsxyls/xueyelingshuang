#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"

class CompetitionPrepareTask : public CTask
{
public:
	CompetitionPrepareTask();

public:
	void DoTask();

	void StopTask();

	void Sleep(int32_t time);

private:
	Semaphore m_sleep;
	std::atomic<bool> m_exit;
};