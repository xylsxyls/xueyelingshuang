#include "stdafx.h"
#include "CwqAllTask.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "D:\\SendToMessageTest.h"
#include "Config.h"

CwqAllTask::CwqAllTask():
m_isEdit(false),
m_exit(false)
{

}

void CwqAllTask::DoTask()
{
	char str[1024] = {};
	::GetWindowTextA(g_config.m_editWnd, str, sizeof(str));
	
	std::string text = (m_isEdit ? str : "");

	if (text.empty())
	{
		CKeyboard::KeyPress('H', 0);
		Sleep(100);
		KeyPressE();
		Sleep(100);
		KeyPressF();
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
		CKeyboard::KeyPress(text[0] + 48, 0);
		KeyPressE();
		CKeyboard::KeyPress('J', 0);
		Sleep(50);
		CKeyboard::KeyPress(text[0] + 48, 0);
		KeyPressE();
		Sleep(50);
		CKeyboard::KeyPress(text[0] + 48, 0);
		KeyPressE();
		Sleep(50);
		CKeyboard::KeyPress(text[0] + 48, 0);
		KeyPressE();
	}
	else if (text.size() == 1)
	{
		CKeyboard::KeyPress(text[0] + 48, 0);
		KeyPressE();
		Sleep(100);
		KeyPressF();
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
		CKeyboard::KeyPress(text[0] + 48, 0);
		KeyPressE();
		CKeyboard::KeyPress('J', 0);
		Sleep(50);
		CKeyboard::KeyPress(text[0] + 48, 0);
		KeyPressE();
		Sleep(50);
		CKeyboard::KeyPress(text[0] + 48, 0);
		KeyPressE();
		Sleep(50); CKeyboard::KeyPress(text[0] + 48, 0);
		KeyPressE();
	}
	else
	{
		CKeyboard::KeyPress(text[0] + 48, 0);
		if (Sleep(100))
		{
			return;
		}
		CKeyboard::KeyPress(text[1] + 48, 0);
		CKeyboard::KeyPress(text[0] + 48, 0);
		KeyPressE();
		Sleep(100);
		KeyPressF();
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
		CKeyboard::KeyPress(text[0] + 48, 0);
		KeyPressE();
		CKeyboard::KeyPress('J', 0);
		Sleep(50);
		CKeyboard::KeyPress(text[0] + 48, 0);
		KeyPressE();
		Sleep(50);
		CKeyboard::KeyPress(text[0] + 48, 0);
		KeyPressE();
		Sleep(50); CKeyboard::KeyPress(text[0] + 48, 0);
		KeyPressE();
	}
}

void CwqAllTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
}

void CwqAllTask::LockHero(const std::string& text)
{
	if (text.size() == 1)
	{
		CKeyboard::KeyPress(text[0] + 48, 0);
	}
	else if (text.size() >= 2)
	{
		CKeyboard::KeyPress(text[0] + 48, 0);
		if (Sleep(100))
		{
			return;
		}
		CKeyboard::KeyPress(text[1] + 48, 0);
		CKeyboard::KeyPress(text[0] + 48, 0);
	}
}

void CwqAllTask::ClickHero(char heroNum)
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

bool CwqAllTask::Sleep(int32_t sleepTime)
{
	::Sleep(sleepTime);
	return false;
	//return m_sleep.wait(sleepTime);
}

void CwqAllTask::KeyPressE()
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

void CwqAllTask::KeyPressF()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(g_config.m_fCenterPoint, 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}

void CwqAllTask::setParam(bool isEdit)
{
	m_isEdit = isEdit;
}