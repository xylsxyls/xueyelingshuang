#include "stdafx.h"
#include "CtrlDTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

void CtrlDTask::DoTask()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::RightUp();
	CMouse::LeftUp();
	CKeyboard::KeyDown('Z');
	Sleep(50);

	disableMouse();

	moveAbsolute(xyls::Point(772, 948), 0);
	CMouse::LeftClick(0);
	Sleep(50);

	moveAbsolute(xyls::Point(1746, 860), 0);
	CMouse::LeftClick(0);
	Sleep(50);

	moveAbsolute(xyls::Point(108, 435), 0);
	CMouse::LeftClick(0);
	Sleep(50);

	moveAbsolute(xyls::Point(662, 458), 0);
	CMouse::LeftDoubleClick(20);
	Sleep(50);

	CKeyboard::KeyUp('Z');
	Sleep(50);

	moveAbsolute(xyls::Point(198, 429), 0);
	CMouse::LeftClick(0);
	Sleep(50);

	moveAbsolute(currentPos, 0);

	enableMouse();

	CMouse::RightDown();
}