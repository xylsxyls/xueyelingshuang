#include "stdafx.h"
#include "CTabRTask.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

void CTabRTask::DoTask()
{
	//CMouse::RightUp();
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(g_config.m_bCenterPoint, 0);
	//Sleep(50);
	CMouse::LeftClick(0);
	//Sleep(50);
	CMouse::MoveAbsolute(currentPos, 0);
}