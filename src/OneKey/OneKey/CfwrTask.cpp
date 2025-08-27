#include "stdafx.h"
#include "CfwrTask.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CStopWatch/CStopWatchAPI.h"
#include "OneKey.h"
#include "D:\\SendToMessageTest.h"
#include "CKeyboardConfig/CKeyboardConfigAPI.h"
#include "Config.h"

CfwrTask::CfwrTask():
m_editIndex(0),
m_exit(false)
{

}

void CfwrTask::DoTask()
{
	char str[1024] = {};
	::GetWindowTextA(g_config.m_editWnd, str, 1024);

	std::string text = str;
	char e = text[m_editIndex];
	if (e == '8')
	{
		e = 'C' - 48;
	}

	if (text.size() <= m_editIndex)
	{
		KeyPressE();
	}
	else
	{
		CKeyboard::KeyPress(e + 48, 0);
	}
	if (Sleep(100))
	{
		return;
	}
	CMouse::RightClick(0);
	if (Sleep(50))
	{
		return;
	}
	//CKeyboard::KeyPress('F', 0);
	KeyPressF();
	if (Sleep(100))
	{
		return;
	}
	CKeyboard::KeyPress('J', 0);
	if (Sleep(10))
	{
		return;
	}
	CKeyboard::KeyPress(e + 48, 0);
	if (Sleep(10))
	{
		return;
	}
	CMouse::RightClick(0);
	if (Sleep(50))
	{
		return;
	}
	CMouse::RightClick(0);
	if (Sleep(50))
	{
		return;
	}
	CMouse::RightClick(0);
	if (Sleep(50))
	{
		return;
	}
	//CKeyboard::KeyPress('F', 0);
	//KeyPressF();
	//Sleep(55);
	//CKeyboard::KeyPress(text[m_editIndex] + 48, 0);
	//CMouse::RightClick(0);
	//Sleep(50);
	KeyPressR();
	if (Sleep(50))
	{
		return;
	}
	//CKeyboard::KeyPress(text[m_editIndex] + 48, 0);
	CMouse::RightClick(0);
	//Sleep(50);
	KeyPressR();
	if (Sleep(50))
	{
		return;
	}
	//CKeyboard::KeyPress(text[m_editIndex] + 48, 0);
	CMouse::RightClick(0);
	//Sleep(50);
	KeyPressR();
	if (Sleep(50))
	{
		return;
	}
	//CKeyboard::KeyPress(text[m_editIndex] + 48, 0);
	CMouse::RightClick(0);
	//Sleep(50);
	KeyPressR();
}

void CfwrTask::StopTask()
{
	m_exit = true;
}

void CfwrTask::KeyPressE()
{
	//if (code1 != 0)
	//{
	//	CKeyboard::KeyPress(code1, 0);
	//}
	//if (code2 != 0)
	{
		CKeyboard::KeyPress('C', 0);
	}
}

void CfwrTask::KeyPressF()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(g_config.m_fCenterPoint, 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}

void CfwrTask::KeyPressR()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(g_config.m_rCenterPoint, 0);
	CMouse::LeftDown();
	Sleep(50);
	CMouse::LeftUp();
	CMouse::MoveAbsolute(currentPos, 0);
}

bool CfwrTask::Sleep(int32_t sleepTime)
{
	std::this_thread::sleep_for(std::chrono::microseconds(sleepTime * 1000));
	return m_exit;
	//return m_sleep.wait(sleepTime);
}

void CfwrTask::setParam(uint32_t editIndex)
{
	m_editIndex = editIndex;
}