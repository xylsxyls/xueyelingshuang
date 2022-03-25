#include "stdafx.h"
#include "CfwqTask.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"

void CfwqTask::DoTask()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(xyls::Point(1464, 784), 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
	Sleep(100);
	CKeyboard::KeyPress('Q', 0);
	
	
	Sleep(550);
	CKeyboard::KeyPress('F', 0);
}