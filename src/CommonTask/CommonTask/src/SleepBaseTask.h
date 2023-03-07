#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include "CommonTaskMacro.h"

class CommonTaskAPI SleepBaseTask : public CTask
{
public:
	SleepBaseTask();

public:
	void StopTask();

	void Sleep(int32_t time);

protected:
	Semaphore m_sleep;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::atomic<bool> m_exit;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};