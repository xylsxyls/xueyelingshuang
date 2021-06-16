#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"

class ConvoyTask : public CTask
{
public:
	ConvoyTask();

public:
	void DoTask();

	void StopTask();

	void setParam(int32_t preSleepTime, int32_t clientIndex, const std::string& placeName, int32_t clickIndex, int32_t findClickTimes);

	void Sleep(int32_t time);

private:
	Semaphore m_sleep;
	int32_t m_preSleepTime;
	int32_t m_clientIndex;
	std::string m_placeName;
	int32_t m_clickIndex;
	int32_t m_findClickTimes;
	std::atomic<bool> m_exit;
};