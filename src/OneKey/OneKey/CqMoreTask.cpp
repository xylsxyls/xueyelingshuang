#include "stdafx.h"
#include "CqMoreTask.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "D:\\SendToMessageTest.h"

CqMoreTask::CqMoreTask():
m_isW(false)
{

}

void CqMoreTask::DoTask()
{
	if (m_isW)
	{
		CKeyboard::KeyUp('W');
		CKeyboard::KeyUp('Q');
	}
	
	Sleep(30);
	if (m_isW)
	{
		CKeyboard::KeyUp('W');
		CKeyboard::KeyUp('Q');
	}
	
	CKeyboard::KeyPress('Q', 0);
	Sleep(20);
	CKeyboard::KeyPress('Q', 0);
}

void CqMoreTask::setParam(bool isW)
{
	m_isW = isW;
}