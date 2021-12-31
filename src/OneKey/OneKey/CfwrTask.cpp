#include "stdafx.h"
#include "CfwrTask.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CStopWatch/CStopWatchAPI.h"
#include "OneKey.h"
#include "D:\\SendToMessageTest.h"

extern int32_t code1;
extern int32_t code2;
extern HWND g_editWnd;
extern CStopWatch keyWatch[256];

CfwrTask::CfwrTask():
m_editIndex(0)
{

}

void CfwrTask::DoTask()
{
	char str[1024] = {};
	::GetWindowTextA(g_editWnd, str, 1024);

	std::string text = str;

	if (text.size() <= m_editIndex)
	{
		KeyPressE();
	}
	else
	{
		CKeyboard::KeyPress(text[m_editIndex] + 48, 0);
	}
	Sleep(100);
	CMouse::RightClick(0);
	Sleep(50);
	//CKeyboard::KeyPress('F', 0);
	KeyPressF();
	Sleep(100);
	CKeyboard::KeyPress('W', 0);
	Sleep(10);
	CKeyboard::KeyPress(text[m_editIndex] + 48, 0);
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
	CMouse::MoveAbsolute(xyls::Point(1224, 957), 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}

void CfwrTask::KeyPressR()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(xyls::Point(1714, 598), 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}

void CfwrTask::setParam(uint32_t editIndex)
{
	m_editIndex = editIndex;
}