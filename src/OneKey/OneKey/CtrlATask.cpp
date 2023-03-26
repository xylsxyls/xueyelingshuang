#include "stdafx.h"
#include "CtrlATask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

void CtrlATask::DoTask()
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
	CKeyboard::KeyDown('O');
	Sleep(100);

	disableMouse();

	moveAbsolute(g_config.m_equipBeginPoint + xyls::Point(equipIndex * g_config.m_equipWidth, 0), 0);
	CMouse::LeftClick(0);
	Sleep(100);

	moveAbsolute(xyls::Point(1746, 860), 0);
	CMouse::LeftClick(0);
	Sleep(100);

	moveAbsolute(xyls::Point(108, 545), 0);
	CMouse::LeftClick(0);
	Sleep(100);

	moveAbsolute(xyls::Point(1364, 601), 0);
	CMouse::LeftDoubleClick(20);
	Sleep(100);

	CKeyboard::KeyUp('O');
	Sleep(100);

	moveAbsolute(xyls::Point(198, 429), 0);
	CMouse::LeftClick(0);

	moveAbsolute(currentPos, 0);

	enableMouse();

	CMouse::RightDown();
}