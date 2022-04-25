#include "stdafx.h"
#include "CtrlSTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

void CtrlSTask::DoTask()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::RightUp();
	CMouse::LeftUp();
	CKeyboard::KeyDown('Z');
	Sleep(50);

	disableMouse();

	moveAbsolute(xyls::Point(772, 948), 0);
	CMouse::LeftClick(0);
	Sleep(80);

	moveAbsolute(xyls::Point(1746, 860), 0);
	CMouse::LeftClick(0);
	Sleep(80);

	moveAbsolute(xyls::Point(108, 435), 0);
	CMouse::LeftClick(0);
	Sleep(80);

	moveAbsolute(xyls::Point(527, 458), 0);//1086, 749
	CMouse::LeftDoubleClick(20);
	Sleep(80);
	CKeyboard::KeyUp('Z');
	Sleep(80);

	moveAbsolute(xyls::Point(198, 429), 0);
	CMouse::LeftClick(0);
	Sleep(80);
	//CMouse::MoveAbsolute(xyls::Point(1732, 424), 0);
	//CMouse::LeftClick(0);

	moveAbsolute(currentPos, 0);

	enableMouse();

	CMouse::RightDown();

	CKeyboard::KeyPress('L', 0);
	Sleep(50);
	CKeyboard::KeyPress('L', 0);
	Sleep(50);
	CKeyboard::KeyPress('L', 0);
}