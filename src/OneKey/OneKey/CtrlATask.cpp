#include "stdafx.h"
#include "CtrlATask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

void CtrlATask::DoTask()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::RightUp();
	CMouse::LeftUp();
	CKeyboard::KeyDown('Z');
	Sleep(80);

	disableMouse();

	moveAbsolute(xyls::Point(772, 948), 0);
	CMouse::LeftClick(0);
	Sleep(80);

	moveAbsolute(xyls::Point(1746, 860), 0);
	CMouse::LeftClick(0);
	Sleep(80);

	moveAbsolute(xyls::Point(108, 545), 0);
	CMouse::LeftClick(0);
	Sleep(80);

	moveAbsolute(xyls::Point(1364, 601), 0);
	CMouse::LeftDoubleClick(20);
	Sleep(80);

	CKeyboard::KeyUp('Z');
	Sleep(80);

	moveAbsolute(xyls::Point(198, 429), 0);
	CMouse::LeftClick(0);

	moveAbsolute(currentPos, 0);

	enableMouse();

	CMouse::RightDown();
}