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
extern CStopWatch oneWatch;
extern CStopWatch twoWatch;
extern CStopWatch threeWatch;
extern CStopWatch spaceWatch;

CwqTask::CwqTask():
m_isR(false),
m_isF(false)
{

}

void CwqTask::DoTask()
{
	if (m_isR)
	{
		LockHero(false);
		CKeyboard::KeyPress('W', 0);
		::Sleep(150);
		KeyPressE();
		::Sleep(100);
		KeyPressF();
		::Sleep(100);
		CKeyboard::KeyPress('Q', 0);
		::Sleep(350);
		KeyPressE();
	}
	else
	{
		//每次按键之后都要锁定，可能因为w导致目标改变
		LockHero(true);
		KeyPressE();
		::Sleep(100);
		if (m_isF)
		{
			KeyPressF();
		}
		::Sleep(50);
		CKeyboard::KeyPress('W', 0);
		::Sleep(50);
		CKeyboard::KeyPress('W', 0);
		::Sleep(50);
		CKeyboard::KeyPress('W', 0);
		::Sleep(50);
		Lock3();
		KeyPressE();
		CKeyboard::KeyPress('W', 0);
		::Sleep(50);
		Lock3();
		KeyPressE();
		::Sleep(50);
		Lock3();
		KeyPressE();
		::Sleep(50);
		Lock3();
		KeyPressE();
		::Sleep(50);
		Lock3();
		KeyPressE();
	}
}

void CwqTask::setParam(bool isR, bool isF)
{
	m_isR = isR;
	m_isF = isF;
}

void CwqTask::KeyPress(int32_t vkCode)
{
	{
		std::shared_ptr<CKeyTask> spKeyTask(new CKeyTask);
		spKeyTask->setParam(vkCode, true);
		CTaskThreadManager::Instance().GetThreadInterface(g_threadId)->PostTask(spKeyTask);
	}

	{
		std::shared_ptr<CKeyTask> spKeyTask(new CKeyTask);
		spKeyTask->setParam(vkCode, false);
		CTaskThreadManager::Instance().GetThreadInterface(g_threadId)->PostTask(spKeyTask);
	}
}

void CwqTask::Sleep(int32_t sleepTime)
{
	std::shared_ptr<CSleepTask> spSleepTask(new CSleepTask);
	spSleepTask->setParam(sleepTime);
	CTaskThreadManager::Instance().GetThreadInterface(g_threadId)->PostTask(spSleepTask);
}

void CwqTask::KeyPressE()
{
	if (code1 != 0)
	{
		CKeyboard::KeyPress(code1, 0);
	}
	if (code2 != 0)
	{
		CKeyboard::KeyPress(code2, 0);
	}
}

void CwqTask::KeyPressF()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(xyls::Point(1224, 957), 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}

void CwqTask::LockHero(bool edit)
{
	if (edit && threeWatch.GetWatchTime() < 1000)
	{
		char text[1024] = {};
		::GetWindowTextA(g_editWnd, text, 1024);
		std::string str = text;
		if (str.size() == 1)
		{
			CKeyboard::KeyPress(str[0] + 48, 0);
			//::Sleep(100);
		}
		else if (str.size() >= 2)
		{
			CKeyboard::KeyPress(str[0] + 48, 0);
			::Sleep(100);
			CKeyboard::KeyPress(str[1] + 48, 0);
			CKeyboard::KeyPress(str[0] + 48, 0);
			//::Sleep(100);
		}
		else
		{
			CKeyboard::KeyPress('H', 0);
			//::Sleep(100);
		}
	}
	if (oneWatch.GetWatchTime() < 1000)
	{
		char text[1024] = {};
		::GetWindowTextA(g_editWnd, text, 1024);
		std::string str = text;
		if (str.size() >= 2)
		{
			CKeyboard::KeyPress(str[1] + 48, 0);
		}
	}
	if (twoWatch.GetWatchTime() < 1000)
	{
		char text[1024] = {};
		::GetWindowTextA(g_editWnd, text, 1024);
		std::string str = text;
		if (!str.empty())
		{
			CKeyboard::KeyPress(str[0] + 48, 0);
		}
	}
}

void CwqTask::Lock3()
{
	if (threeWatch.GetWatchTime() < 1000)
	{
		char text[1024] = {};
		::GetWindowTextA(g_editWnd, text, 1024);
		std::string str = text;
		if (str.size() >= 1)
		{
			CKeyboard::KeyPress(str[0] + 48, 0);
			//::Sleep(100);
		}
	}
}