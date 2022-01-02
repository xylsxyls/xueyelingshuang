#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "Point/PointAPI.h"

class MoveClickTask : public CTask
{
public:
	MoveClickTask();

public:
	void DoTask();

	void setParam(const xyls::Point& point, int32_t beforeSleepTime = 0);

private:
	int32_t m_beforeSleepTime;
	xyls::Point m_point;
};