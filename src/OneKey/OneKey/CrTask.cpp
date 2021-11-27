#include "stdafx.h"
#include "CrTask.h"
#include "CMouse/CMouseAPI.h"

void CrTask::DoTask()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(xyls::Point(1740, 626), 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}