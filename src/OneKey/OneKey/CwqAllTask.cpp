#include "stdafx.h"
#include "CwqAllTask.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "D:\\SendToMessageTest.h"

extern uint32_t g_threadId;
extern int32_t code1;
extern int32_t code2;
extern bool rightDown;
extern HWND g_editWnd;

CwqAllTask::CwqAllTask():
m_isEdit(false),
m_exit(false)
{

}

void CwqAllTask::DoTask()
{
	char str[1024] = {};
	::GetWindowTextA(g_editWnd, str, 1024);
	
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
		CKeyboard::KeyPress(text[0] + 48, 0);
		KeyPressE();
		CKeyboard::KeyPress('W', 0);
		Sleep(50);
		CKeyboard::KeyPress(text[0] + 48, 0);
		KeyPressE();
		Sleep(50);
		CKeyboard::KeyPress(text[0] + 48, 0);
		KeyPressE();
		Sleep(50); CKeyboard::KeyPress(text[0] + 48, 0);
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
		CKeyboard::KeyPress(text[0] + 48, 0);
		KeyPressE();
		CKeyboard::KeyPress('W', 0);
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
		CKeyboard::KeyPress(text[0] + 48, 0);
		KeyPressE();
		CKeyboard::KeyPress('W', 0);
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
	RCSend("stop all");
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
		point = xyls::Point(1276 +40, 523 + 40);
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

bool CwqAllTask::Sleep(int32_t sleepTime)
{
	::Sleep(sleepTime);
	return false;
	//return m_sleep.wait(sleepTime);
}

void CwqAllTask::KeyPressE()
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

void CwqAllTask::KeyPressF()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(xyls::Point(1224, 957), 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}

void CwqAllTask::setParam(bool isEdit)
{
	m_isEdit = isEdit;
}