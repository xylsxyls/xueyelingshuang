#include "stdafx.h"
#include "CtrlFTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

void CtrlFTask::DoTask()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::RightUp();
	CMouse::LeftUp();
	CKeyboard::KeyUp('F');

	disableMouse();

	moveAbsolute(xyls::Point(67, 451), 0);
	CMouse::LeftClick(0);
	Sleep(50);

	moveAbsolute(xyls::Point(772, 948), 0);
	CMouse::LeftClick(0);
	Sleep(50);

	moveAbsolute(xyls::Point(1746, 860), 0);
	CMouse::LeftClick(0);
	Sleep(50);

	moveAbsolute(xyls::Point(99, 328), 0);
	CMouse::LeftClick(0);
	Sleep(50);

	moveAbsolute(xyls::Point(524, 460), 0);
	CMouse::LeftDoubleClick(20);
	Sleep(50);

	CKeyboard::KeyPress('Z', 0);
	Sleep(50);

	moveAbsolute(xyls::Point(198, 429), 0);
	CMouse::LeftClick(0);
	Sleep(50);

	moveAbsolute(currentPos, 0);

	enableMouse();

	CMouse::RightDown();
}