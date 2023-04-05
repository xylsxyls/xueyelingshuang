#include "stdafx.h"
#include "SmySpaceTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"
#include "CKeyboardConfig/CKeyboardConfigAPI.h"

void SmySpaceTask::DoTask()
{
	CKeyboard::KeyPress('H', 0);
	Sleep(50);

	//xyls::Point currentPos = CMouse::GetCurrentPos();
	//CMouse::MoveAbsolute(g_config.m_bCenterPoint, 0);
	//CMouse::LeftClick(0);
	//CMouse::MoveAbsolute(currentPos, 0);

	//CKeyboard::KeyUp(TAB);

	CMouse::RightUp();
}