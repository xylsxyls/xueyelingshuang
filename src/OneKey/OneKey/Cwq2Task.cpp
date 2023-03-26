#include "stdafx.h"
#include "Cwq2Task.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "D:\\SendToMessageTest.h"
#include "Config.h"

Cwq2Task::Cwq2Task():
m_editIndex(0),
m_exit(false)
{

}

void Cwq2Task::DoTask()
{
	char str[1024] = {};
	::GetWindowTextA(g_config.m_editWnd, str, 1024);

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

		CKeyboard::KeyPress('J', 0);
		if (Sleep(150))
		{
			return;
		}
		KeyPressE();
	}
	else
	{
		char e = text[m_editIndex];
		if (e == '8')
		{
			e = 'C' - 48;
		}
		CKeyboard::KeyPress(e + 48, 0);
		Sleep(100);
		//KeyPressE();
		//Sleep(100);
		//KeyPressF();
		CKeyboard::KeyPress('F', 0);
		if (Sleep(100))
		{
			return;
		}
		//CKeyboard::KeyPress('W', 0);
		//if (Sleep(50))
		//{
		//	return;
		//}
		//CKeyboard::KeyPress('W', 0);
		//if (Sleep(50))
		//{
		//	return;
		//}
		CKeyboard::KeyPress('J', 0);
		if (Sleep(50))
		{
			return;
		}
		CKeyboard::KeyPress('J', 0);
		if (Sleep(50))
		{
			return;
		}
		CKeyboard::KeyPress('J', 0);
		if (Sleep(50))
		{
			return;
		}
		CKeyboard::KeyPress('J', 0);
		if (Sleep(50))
		{
			return;
		}
		CKeyboard::KeyPress('J', 0);
		if (Sleep(50))
		{
			return;
		}
		//ClickHero(m_editIndex + 48 + 1);
		CKeyboard::KeyPress(e + 48, 0);
		//KeyPressE();
		Sleep(50);
		//ClickHero(m_editIndex + 48 + 1);
		CKeyboard::KeyPress(e + 48, 0);
		//KeyPressE();
		Sleep(50);
		//ClickHero(m_editIndex + 48 + 1);
		CKeyboard::KeyPress(e + 48, 0);
		//KeyPressE();
		Sleep(50);
		//ClickHero(m_editIndex + 48 + 1);
		CKeyboard::KeyPress(e + 48, 0);
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
	if (g_config.m_code1 != 0)
	{
		//CKeyboard::KeyPress(code1, 0);
	}
	if (g_config.m_code2 != 0)
	{
		CKeyboard::KeyPress('C', 0);
	}
}

void Cwq2Task::KeyPressF()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(g_config.m_fCenterPoint, 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}

void Cwq2Task::ClickHero(char heroNum)
{
	if (heroNum < '1' || heroNum > '5')
	{
		KeyPressE();
		return;
	}
	xyls::Rect point = g_config.m_heroHeadPoint[heroNum - 49] + xyls::Point(g_config.m_side / 2, g_config.m_side / 2);
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(point, 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}

void Cwq2Task::setParam(uint32_t editIndex)
{
	m_editIndex = editIndex;
}