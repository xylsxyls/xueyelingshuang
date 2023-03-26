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
m_editIndex(0)
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
	Sleep(100);
	CMouse::RightClick(0);
	Sleep(50);
	//CKeyboard::KeyPress('F', 0);
	KeyPressF();
	Sleep(100);
	CKeyboard::KeyPress('J', 0);
	Sleep(10);
	CKeyboard::KeyPress(e + 48, 0);
	Sleep(10);
	CMouse::RightClick(0);
	Sleep(50);
	CMouse::RightClick(0);
	Sleep(50);
	CMouse::RightClick(0);
	Sleep(50);
	//CKeyboard::KeyPress('F', 0);
	//KeyPressF();
	//Sleep(55);
	//CKeyboard::KeyPress(text[m_editIndex] + 48, 0);
	//CMouse::RightClick(0);
	//Sleep(50);
	KeyPressR();
	Sleep(50);
	//CKeyboard::KeyPress(text[m_editIndex] + 48, 0);
	CMouse::RightClick(0);
	//Sleep(50);
	KeyPressR();
	Sleep(50);
	//CKeyboard::KeyPress(text[m_editIndex] + 48, 0);
	CMouse::RightClick(0);
	//Sleep(50);
	KeyPressR();
	Sleep(50);
	//CKeyboard::KeyPress(text[m_editIndex] + 48, 0);
	CMouse::RightClick(0);
	//Sleep(50);
	KeyPressR();
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

void CfwrTask::setParam(uint32_t editIndex)
{
	m_editIndex = editIndex;
}