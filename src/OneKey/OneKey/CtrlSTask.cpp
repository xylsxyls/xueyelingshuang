#include "stdafx.h"
#include "CtrlSTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

extern xyls::Point equipPoint[6];
extern HWND g_editWnd;

void CtrlSTask::DoTask()
{
	char str[1024] = {};
	::GetWindowTextA(g_editWnd, str, 1024);

	std::string text = str;
	if (text.size() <= 5)
	{
		return;
	}

	char equipIndex = text[5] - 48 - 1;

	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::RightUp();
	CMouse::LeftUp();
	CKeyboard::KeyDown('Z');
	Sleep(100);

	disableMouse();

	moveAbsolute(equipPoint[equipIndex], 0);
	CMouse::LeftClick(0);
	Sleep(100);

	moveAbsolute(xyls::Point(1746, 860), 0);
	CMouse::LeftClick(0);
	Sleep(100);

	moveAbsolute(xyls::Point(108, 435), 0);
	CMouse::LeftClick(0);
	Sleep(100);

	moveAbsolute(xyls::Point(527, 458), 0);//1086, 749
	CMouse::LeftDoubleClick(20);
	Sleep(100);
	CKeyboard::KeyUp('Z');
	Sleep(100);

	moveAbsolute(xyls::Point(198, 429), 0);
	CMouse::LeftClick(0);
	Sleep(100);
	//CMouse::MoveAbsolute(xyls::Point(1732, 424), 0);
	//CMouse::LeftClick(0);

	moveAbsolute(currentPos, 0);

	enableMouse();

	CMouse::RightDown();

	CKeyboard::KeyPress('L', 0);
	Sleep(100);
	CKeyboard::KeyPress('L', 0);
	Sleep(100);
	CKeyboard::KeyPress('L', 0);
}