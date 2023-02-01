#include "stdafx.h"
#include "CRqwTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "D:\\SendToMessageTest.h"
#include "CStopWatch/CStopWatchAPI.h"

extern HWND g_editWnd;
extern xyls::Point g_heroHeadPoint[5];
extern int32_t g_side;
extern int32_t code2;
extern CStopWatch code2Watch;

CRqwTask::CRqwTask():
m_press(0),
m_hasS(false)
{

}

void CRqwTask::DoTask()
{
	char str[1024] = {};
	::GetWindowTextA(g_editWnd, str, 1024);
	std::string text = str;

	int32_t editIndex = -1;
	if (m_press == '1')
	{
		editIndex = 4;
	}
	else if (m_press == '2')
	{
		editIndex = 1;
	}
	else if (m_press == '3')
	{
		editIndex = 0;
	}
	else if (m_press == '4')
	{
		editIndex = 2;
	}
	else if (m_press == '5')
	{
		editIndex = 3;
	}

	char realPress = (editIndex == -1 ? '8' : text[editIndex]);

	if (realPress != '8')
	{
		if (code2 == 'C')
		{
			code2Watch.SetWatchTime(0);
		}
		code2 = 48 + realPress;
	}
	
	if (m_press == 'E')
	{
		CKeyboard::KeyPress('X', 0);
	}
	else if (m_press == 'W')
	{
		CKeyboard::KeyPress('K', 0);
		Sleep(50);
		//CKeyboard::KeyPress('F', 0);
		Sleep(120);
		CKeyboard::KeyPress('X', 0);
		Sleep(250);
		CKeyboard::KeyPress('J', 0);
		Sleep(250);
		CKeyboard::KeyPress('J', 0);
		Sleep(250);
		CKeyboard::KeyPress('J', 0);
	}
	else if (m_press == '1' || m_press == '2' || m_press == '3' || m_press == '4' || m_press == '5')
	{
		if (m_hasS)
		{
			CKeyboard::KeyPress('F', 0);
			Sleep(120);
			CKeyboard::KeyPress('S', 0);
			Sleep(20);
			pressK(realPress);
			Sleep(100);
			CKeyboard::KeyPress('X', 0);
			Sleep(250);
			CKeyboard::KeyPress('J', 0);
			Sleep(250);
			CKeyboard::KeyPress('J', 0);
			Sleep(250);
			CKeyboard::KeyPress('J', 0);
		}
		else
		{
			pressK(realPress);
			Sleep(50);
			CKeyboard::KeyPress('F', 0);
			Sleep(120);
			CKeyboard::KeyPress('X', 0);
			//Sleep(250);
			//CKeyboard::KeyPress('J', 0);
			//Sleep(250);
			//CKeyboard::KeyPress('J', 0);
			//Sleep(250);
			//CKeyboard::KeyPress('J', 0);
		}
	}
}

void CRqwTask::setParam(char press, bool hasS)
{
	m_press = press;
	m_hasS = hasS;
}

void CRqwTask::pressK(char realPress)
{
	if (realPress == '8')
	{
		CKeyboard::KeyPress('K', 0);
	}
	else
	{
		xyls::Point currentPos = CMouse::GetCurrentPos();
		CMouse::MoveAbsolute(xyls::Point(1419, 941), 0);
		CMouse::LeftDown();
		xyls::Point heroHeadPoint = g_heroHeadPoint[realPress - 49];
		CMouse::MoveAbsolute(xyls::Point(heroHeadPoint.x() + g_side / 2, heroHeadPoint.y() + g_side / 2), 0);
		CMouse::LeftUp();
		CMouse::MoveAbsolute(currentPos, 0);
	}
}