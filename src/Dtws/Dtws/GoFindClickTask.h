#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"

class GoFindClickTask : public CTask
{
public:
	GoFindClickTask();

public:
	void DoTask();

	void StopTask();

	void setParam(int32_t preSleepTime, int32_t clientIndex, const std::string& placeName, int32_t clickIndex, int32_t findClickTimes);

	void Sleep(int32_t time);

protected:
	Semaphore m_sleep;
	int32_t m_preSleepTime;
	int32_t m_clientIndex;
	std::string m_placeName;
	int32_t m_clickIndex;
	int32_t m_findClickTimes;
	std::atomic<bool> m_exit;
};