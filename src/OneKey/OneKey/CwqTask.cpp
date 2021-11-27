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

CwqTask::CwqTask():
m_isR(false)
{

}

void CwqTask::DoTask()
{
	if (m_isR)
	{
		CKeyboard::KeyPress('W', 0);
		::Sleep(150);
		KeyPressE(false);
		::Sleep(100);
		CKeyboard::KeyPress('F', 0);
		::Sleep(100);
		CKeyboard::KeyPress('Q', 0);
		::Sleep(350);
		KeyPressE(false);
	}
	else
	{
		KeyPressE(true);
		::Sleep(100);
		CKeyboard::KeyPress('F', 0);
		::Sleep(100);
		CKeyboard::KeyPress('W', 0);
		::Sleep(150);
		KeyPressE(true);
	}
	
}

void CwqTask::setParam(bool isR)
{
	m_isR = isR;
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

void CwqTask::KeyPressE(bool edit)
{
	if (edit)
	{
		if (threeWatch.GetWatchTime() < 1000)
		{
			char text[1024] = {};
			::GetWindowTextA(g_editWnd, text, 1024);
			std::string str = text;
			if (!str.empty())
			{
				CKeyboard::KeyPress(str[0] + 48, 0);
			}
		}
		if (twoWatch.GetWatchTime() < 1000)
		{
			char text[1024] = {};
			::GetWindowTextA(g_editWnd, text, 1024);
			std::string str = text;
			if (str.size() >= 2)
			{
				CKeyboard::KeyPress(str[1] + 48, 0);
			}
		}
		if (oneWatch.GetWatchTime() < 1000)
		{
			char text[1024] = {};
			::GetWindowTextA(g_editWnd, text, 1024);
			std::string str = text;
			if (str.size() >= 3)
			{
				CKeyboard::KeyPress(str[2] + 48, 0);
			}
		}
	}
	
	if (code1 != 0)
	{
		RCSend("code1 = %d", code1);
		CKeyboard::KeyPress(code1, 0);
	}
	if (code2 != 0)
	{
		CKeyboard::KeyPress(code2, 0);
	}
}