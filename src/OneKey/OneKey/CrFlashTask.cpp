#include "stdafx.h"
#include "CrFlashTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

CrFlashTask::CrFlashTask():
m_isFlash(false)
{

}

void CrFlashTask::DoTask()
{
	if (m_isFlash)
	{
		CKeyboard::InputString("re", 0);
		CKeyboard::KeyDown('D');
		CKeyboard::KeyUp('D');
		//CKeyboard::InputString("r", 0);
		Sleep(350);
		CMouse::RightManyClick(6, 250);
		CKeyboard::InputString("qfqqqqq", 50);
		return;
	}
	CKeyboard::InputString("w", 0);
	CKeyboard::InputString("r", 0);
	CMouse::RightManyClick(6, 300);
	Sleep(200);
	CKeyboard::InputString("e", 0);
	Sleep(100);
	CKeyboard::InputString("q", 0);
	CKeyboard::InputString("qqqqqq", 50);
}

void CrFlashTask::setParam(bool isFlash)
{
	m_isFlash = isFlash;
}