#include "stdafx.h"
#include "CfwrTask.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CStopWatch/CStopWatchAPI.h"
#include "OneKey.h"
#include "D:\\SendToMessageTest.h"

extern int32_t code1;
extern int32_t code2;
extern HWND g_editWnd;
extern CStopWatch keyWatch[256];

void CfwrTask::DoTask()
{
	KeyPressE();
	Sleep(100);
	CMouse::RightClick(0);
	Sleep(150);
	CKeyboard::KeyPress('W', 0);
	Sleep(100);
	KeyPressF();
	Sleep(135);
	KeyPressR();
	Sleep(50);
	KeyPressR();
	Sleep(50);
	KeyPressR();
	Sleep(50);
	KeyPressR();
	Sleep(50);
	KeyPressR();
}

void CfwrTask::KeyPressE()
{
	if (keyWatch[VK_SPACE].GetWatchTime() > 3000)
	{
		if (keyWatch['5'].GetWatchTime() < 1000)
		{
			char text[1024] = {};
			::GetWindowTextA(g_editWnd, text, 1024);
			std::string str = text;
			if (!str.empty())
			{
				CKeyboard::KeyPress(str[0] + 48, 0);
			}
		}
		if (keyWatch['4'].GetWatchTime() < 1000)
		{
			char text[1024] = {};
			::GetWindowTextA(g_editWnd, text, 1024);
			std::string str = text;
			if (str.size() >= 2)
			{
				CKeyboard::KeyPress(str[1] + 48, 0);
			}
		}
	}

	if (code1 != 0)
	{
		CKeyboard::KeyPress(code1, 0);
	}
	if (code2 != 0)
	{
		CKeyboard::KeyPress(code2, 0);
	}
}

void CfwrTask::KeyPressF()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(xyls::Point(1224, 957), 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}

void CfwrTask::KeyPressR()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(xyls::Point(1714, 598), 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}