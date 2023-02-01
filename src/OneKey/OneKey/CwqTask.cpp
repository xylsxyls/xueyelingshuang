#include "stdafx.h"
#include "CwqTask.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "D:\\SendToMessageTest.h"
#include "CKeyTask.h"
#include "CSleepTask.h"
#include "CMouse/CMouseAPI.h"
#include "CStopWatch/CStopWatchAPI.h"

extern uint32_t g_threadId;
extern int32_t code1;
extern int32_t code2;
extern bool rightDown;
extern HWND g_editWnd;

CwqTask::CwqTask():
m_isR(false),
m_isF(false),
m_exit(false),
m_isLast(false)
{

}

void CwqTask::DoTask()
{
	if (m_isR)
	{
		//if (LockHero())
		//{
		//	return;
		//}
		//Sleep(150);
		CKeyboard::KeyPress('J', 0);
		Sleep(150);
		KeyPressE();
		Sleep(100);
		//KeyPressF();
		Sleep(100);
		CKeyboard::KeyPress('Q', 0);
		Sleep(350);
		KeyPressE();
	}
	else
	{
		//每次按键之后都要锁定，可能因为w导致目标改变
		//if (LockHero())
		//{
		//	RCSend("end");
		//	return;
		//}
		//KeyPressE();
		//if (Sleep(100))
		//{
		//	RCSend("end100");
		//	return;
		//}
		//
		//if (m_isF)
		//{
		//	KeyPressF();
		//}
		//if (Sleep(50))
		//{
		//	RCSend("end50");
		//	return;
		//}
		//RCSend("w");
		//CKeyboard::KeyPress('W', 0);
		//if (Sleep(50))
		//{
		//	RCSend("end501");
		//	return;
		//}
		//CKeyboard::KeyPress('W', 0);
		//if (Sleep(50))
		//{
		//	RCSend("end502");
		//	return;
		//}
		//CKeyboard::KeyPress('W', 0);
		//if (Sleep(50))
		//{
		//	RCSend("end503");
		//	return;
		//}
		//Lock3();
		//KeyPressE();
		//CKeyboard::KeyPress('W', 0);
		//Sleep(50);
		//Lock3();
		//KeyPressE();
		//Sleep(50);
		//Lock3();
		//KeyPressE();
		//Sleep(50);
		//Lock3();
		//KeyPressE();
		//Sleep(50);
		//Lock3();
		//KeyPressE();
	}
}

void CwqTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
}

void CwqTask::setParam(bool isR, bool isF, int32_t key, bool isLast)
{
	m_isR = isR;
	m_isF = isF;
	m_key = key;
	m_isLast = isLast;
}

bool CwqTask::Sleep(int32_t sleepTime)
{
	return m_sleep.wait(sleepTime);
}

void CwqTask::KeyPressE()
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

void CwqTask::KeyPressF()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(xyls::Point(1224, 957), 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}

bool CwqTask::LockHero()
{
	switch (m_key)
	{
	case 0:
	{

	}
	break;
	case 2:
	{
		CKeyboard::KeyPress('H', 0);
		if (Sleep(100))
		{
			return true;
		}
	}
	break;
	case 3:
	{
		if (!m_isR)
		{
			char text[1024] = {};
			::GetWindowTextA(g_editWnd, text, 1024);
			std::string str = text;
			if (str.size() == 1)
			{
				if (m_isLast)
				{
					CKeyboard::KeyPress('H', 0);
				}
				else
				{
					CKeyboard::KeyPress(str[0] + 48, 0);
				}
				if (Sleep(100))
				{
					return true;
				}
			}
			else if (str.size() >= 2)
			{
				if (m_isLast)
				{
					CKeyboard::KeyPress(str[1] + 48, 0);
					if (Sleep(100))
					{
						return true;
					}
				}
				else
				{
					CKeyboard::KeyPress(str[0] + 48, 0);
					if (Sleep(100))
					{
						return true;
					}
					CKeyboard::KeyPress(str[1] + 48, 0);
					CKeyboard::KeyPress(str[0] + 48, 0);
				}
			}
			else
			{
				CKeyboard::KeyPress('H', 0);
				if (Sleep(100))
				{
					return true;
				}
			}
		}
	}
	break;
	default:
		break;
	}
	return false;
}

void CwqTask::Lock3()
{
	//if (threeWatch.GetWatchTime() < 1000)
	//{
	//	char text[1024] = {};
	//	::GetWindowTextA(g_editWnd, text, 1024);
	//	std::string str = text;
	//	if (str.size() == 1)
	//	{
	//		if (m_isLast)
	//		{
	//			CKeyboard::KeyPress('H', 0);
	//		}
	//		else
	//		{
	//			CKeyboard::KeyPress(str[0] + 48, 0);
	//		}
	//	}
	//	else if (str.size() >= 2)
	//	{
	//		if (m_isLast)
	//		{
	//			CKeyboard::KeyPress(str[1] + 48, 0);
	//		}
	//		else
	//		{
	//			CKeyboard::KeyPress(str[0] + 48, 0);
	//		}
	//	}
	//}
}