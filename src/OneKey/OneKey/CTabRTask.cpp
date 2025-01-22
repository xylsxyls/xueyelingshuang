#include "stdafx.h"
#include "CTabRTask.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"
#include "CKeyboard/CKeyboardAPI.h"

void CTabRTask::DoTask()
{
	//CMouse::RightUp();
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(g_config.m_bCenterPoint, 0);
	//Sleep(50);
	CMouse::LeftClick(0);
	//Sleep(50);
	CMouse::MoveAbsolute(currentPos, 0);
	Sleep(50);
	CKeyboard::KeyPress(VK_LWIN, 0);
	Sleep(20);
	CKeyboard::KeyPress(VK_LWIN, 0);
}