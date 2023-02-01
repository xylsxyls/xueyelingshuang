#include "stdafx.h"
#include "CHoldRTask.h"
#include "CMouse/CMouseAPI.h"
#include "CStopWatch/CStopWatchAPI.h"

CHoldRTask::CHoldRTask():
m_isDown(false),
m_tabStopWatch(nullptr)
{

}

void CHoldRTask::DoTask()
{
	if (m_isDown)
	{
		if (m_tabStopWatch->GetWatchTime() > 500)
		{
			CMouse::RightUp();
			CMouse::MoveAbsolute(xyls::Point(1742, 634), 0);
			CMouse::LeftDown();
		}
	}
	else
	{
		if (m_tabStopWatch->GetWatchTime() > 1000)
		{
			CMouse::LeftUp();
		}
	}
}

void CHoldRTask::setParam(bool isDown, CStopWatch* tabStopWatch)
{
	m_isDown = isDown;
	m_tabStopWatch = tabStopWatch;
}