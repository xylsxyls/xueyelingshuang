#include "stdafx.h"
#include "CrTask.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

void CrTask::DoTask()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(g_config.m_rCenterPoint, 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}