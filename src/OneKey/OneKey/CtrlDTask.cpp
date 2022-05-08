#include "stdafx.h"
#include "CtrlDTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

extern xyls::Point equipPoint[6];
extern HWND g_editWnd;

void CtrlDTask::DoTask()
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
	Sleep(50);

	disableMouse();

	moveAbsolute(equipPoint[equipIndex], 0);
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