#include "stdafx.h"
#include "CtrlFTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

void CtrlFTask::DoTask()
{
	char str[1024] = {};
	::GetWindowTextA(g_config.m_editWnd, str, 1024);

	std::string text = str;
	if (text.size() <= 5)
	{
		return;
	}

	char equipIndex = text[5] - 48 - 1;

	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::RightUp();
	CMouse::LeftUp();
	CKeyboard::KeyUp('F');

	disableMouse();

	moveAbsolute(xyls::Point(67, 451), 0);
	CMouse::LeftClick(0);
	Sleep(50);

	moveAbsolute(g_config.m_equipBeginPoint + xyls::Point(equipIndex * g_config.m_equipWidth, 0), 0);
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

	CKeyboard::KeyPress('O', 0);
	Sleep(50);

	moveAbsolute(xyls::Point(198, 429), 0);
	CMouse::LeftClick(0);
	Sleep(50);

	moveAbsolute(currentPos, 0);

	enableMouse();

	CMouse::RightDown();
}