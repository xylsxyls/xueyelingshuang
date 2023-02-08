#include "stdafx.h"
#include "CwrTask.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "Config.h"

void CwrTask::DoTask()
{
	CKeyboard::KeyPress('H', 0);
	Sleep(50);
	CKeyboard::KeyDown('J');
	Sleep(10);
	CKeyboard::KeyUp('J');
	Sleep(100);
	KeyPressF();
	Sleep(100);
	KeyPressR();
	Sleep(100);
	KeyPressR();
	Sleep(100);
	KeyPressR();
}

void CwrTask::KeyPressF()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(g_config.m_fCenterPoint, 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}

void CwrTask::KeyPressR()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(g_config.m_rCenterPoint, 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}