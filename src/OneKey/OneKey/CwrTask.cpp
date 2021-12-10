#include "stdafx.h"
#include "CwrTask.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyBoard/CKeyboardAPI.h"

void CwrTask::DoTask()
{
	CKeyboard::KeyPress('H', 0);
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
	CMouse::MoveAbsolute(xyls::Point(1224, 957), 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}

void CwrTask::KeyPressR()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(xyls::Point(1714, 598), 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}