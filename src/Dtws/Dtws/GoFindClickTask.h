#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include <vector>
#include "Point/PointAPI.h"

class GoFindClickTask : public CTask
{
public:
	GoFindClickTask();

public:
	void DoTask();

	void StopTask();

	void setParam(int32_t preSleepTime,
		int32_t clientIndex,
		const std::string& placeName,
		int32_t clickIndex,
		const std::vector<xyls::Point>& acceptPoint);

	void Sleep(int32_t time);

protected:
	Semaphore m_sleep;
	int32_t m_preSleepTime;
	int32_t m_clientIndex;
	std::string m_placeName;
	int32_t m_clickIndex;
	std::vector<xyls::Point> m_acceptPoint;
	std::atomic<bool> m_exit;
};