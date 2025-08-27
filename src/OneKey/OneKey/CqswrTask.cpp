#include "stdafx.h"
#include "CqswrTask.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "Config.h"

CqswrTask::CqswrTask() :
m_editIndex(0),
m_exit(false)
{

}

void CqswrTask::DoTask()
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
		CKeyboard::KeyPress('Z', 0);
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
		if (Sleep(50))
		{
			return;
		}
		CKeyboard::KeyPress('L', 0);
		if (Sleep(10))
		{
			return;
		}
		CKeyboard::KeyPress('X', 0);
	}
	else
	{
		char e = text[m_editIndex];
		if (e == '8')
		{
			e = 'C' - 48;
		}
		CKeyboard::KeyPress(e + 48, 0);
		if (Sleep(140))
		{
			return;
		}
		CKeyboard::KeyPress('Z', 0);
		if (Sleep(140))
		{
			return;
		}
		CKeyboard::KeyPress('J', 0);
		if (Sleep(140))
		{
			return;
		}
		CKeyboard::KeyPress(e + 48, 0);
		if (Sleep(190))
		{
			return;
		}
		CKeyboard::KeyPress('L', 0);
		if (Sleep(30))
		{
			return;
		}
		CKeyboard::KeyPress('N', 0);
	}
}

void CqswrTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
}

bool CqswrTask::Sleep(int32_t sleepTime)
{
	std::this_thread::sleep_for(std::chrono::microseconds(sleepTime * 1000));
	return m_exit;
}

void CqswrTask::KeyPressE()
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

void CqswrTask::KeyPressF()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(g_config.m_fCenterPoint, 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}

void CqswrTask::ClickHero(char heroNum)
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

void CqswrTask::setParam(uint32_t editIndex)
{
	m_editIndex = editIndex;
}