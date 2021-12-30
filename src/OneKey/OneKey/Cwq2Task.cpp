#include "stdafx.h"
#include "Cwq2Task.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "D:\\SendToMessageTest.h"

extern uint32_t g_threadId;
extern int32_t code1;
extern int32_t code2;
extern bool rightDown;
extern HWND g_editWnd;

Cwq2Task::Cwq2Task():
m_editIndex(0),
m_exit(false)
{

}

void Cwq2Task::DoTask()
{
	char str[1024] = {};
	::GetWindowTextA(g_editWnd, str, 1024);

	std::string text = str;

	if (text.size() <= m_editIndex)
	{
		KeyPressE();
		if (Sleep(80))
		{
			return;
		}
		CKeyboard::KeyPress('F', 0);
		if (Sleep(100))
		{
			return;
		}

		CKeyboard::KeyPress('W', 0);
		if (Sleep(150))
		{
			return;
		}
		KeyPressE();
	}
	else
	{
		CKeyboard::KeyPress(text[m_editIndex] + 48, 0);
		Sleep(50);
		//KeyPressE();
		//Sleep(100);
		//KeyPressF();
		CKeyboard::KeyPress('F', 0);
		if (Sleep(50))
		{
			return;
		}
		CKeyboard::KeyPress('W', 0);
		if (Sleep(50))
		{
			return;
		}
		CKeyboard::KeyPress('W', 0);
		if (Sleep(50))
		{
			return;
		}
		CKeyboard::KeyPress('W', 0);
		if (Sleep(50))
		{
			return;
		}
		CKeyboard::KeyPress('W', 0);
		if (Sleep(50))
		{
			return;
		}
		CKeyboard::KeyPress('W', 0);
		if (Sleep(50))
		{
			return;
		}
		//ClickHero(m_editIndex + 48 + 1);
		CKeyboard::KeyPress(text[m_editIndex] + 48, 0);
		//KeyPressE();
		CKeyboard::KeyPress('W', 0);
		Sleep(50);
		//ClickHero(m_editIndex + 48 + 1);
		CKeyboard::KeyPress(text[m_editIndex] + 48, 0);
		//KeyPressE();
		Sleep(50);
		//ClickHero(m_editIndex + 48 + 1);
		CKeyboard::KeyPress(text[m_editIndex] + 48, 0);
		//KeyPressE();
		Sleep(50);
		//ClickHero(m_editIndex + 48 + 1);
		CKeyboard::KeyPress(text[m_editIndex] + 48, 0);
		//KeyPressE();
	}
}

void Cwq2Task::StopTask()
{
	RCSend("stop");
	m_exit = true;
	m_sleep.signal();
}

bool Cwq2Task::Sleep(int32_t sleepTime)
{
	::Sleep(sleepTime);
	return false;
}

void Cwq2Task::KeyPressE()
{
	if (code1 != 0)
	{
		//CKeyboard::KeyPress(code1, 0);
	}
	if (code2 != 0)
	{
		CKeyboard::KeyPress('C', 0);
	}
}

void Cwq2Task::KeyPressF()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(xyls::Point(1224, 957), 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}

void Cwq2Task::ClickHero(char heroNum)
{
	xyls::Rect point;
	if (heroNum == '1')
	{
		point = xyls::Point(1164 + 40, 748 + 40);
	}
	else if (heroNum == '2')
	{
		point = xyls::Point(1202 + 40, 631 + 40);
	}
	else if (heroNum == '3')
	{
		point = xyls::Point(1276 + 40, 523 + 40);
	}
	else if (heroNum == '4')
	{
		point = xyls::Point(1376 + 40, 444 + 40);
	}
	else if (heroNum == '5')
	{
		point = xyls::Point(1489 + 40, 389 + 40);
	}
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(point, 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}

void Cwq2Task::setParam(uint32_t editIndex)
{
	m_editIndex = editIndex;
}