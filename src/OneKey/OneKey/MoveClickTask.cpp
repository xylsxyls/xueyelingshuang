#include "stdafx.h"
#include "MoveClickTask.h"
#include "CMouse/CMouseAPI.h"

MoveClickTask::MoveClickTask():
m_beforeSleepTime(0)
{

}

void MoveClickTask::DoTask()
{
	if (m_beforeSleepTime != 0)
	{
		::Sleep(m_beforeSleepTime);
	}
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(m_point, 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}

void MoveClickTask::setParam(const xyls::Point& point, int32_t beforeSleepTime)
{
	m_point = point;
	m_beforeSleepTime = beforeSleepTime;
}