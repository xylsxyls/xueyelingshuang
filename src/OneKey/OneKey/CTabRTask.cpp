#include "stdafx.h"
#include "CTabRTask.h"
#include "CMouse/CMouseAPI.h"

extern xyls::Point bCenterPoint;

void CTabRTask::DoTask()
{
	//CMouse::RightUp();
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(bCenterPoint, 0);
	//Sleep(50);
	CMouse::LeftClick(0);
	//Sleep(50);
	CMouse::MoveAbsolute(currentPos, 0);
}