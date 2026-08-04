#include "stdafx.h"
#include "CqfwrTask.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "D:\\SendToMessageTest.h"

void CqfwrTask::DoTask()
{
	CKeyboard::KeyPress('K', 0);
	::Sleep(20);
	//CKeyboard::KeyPress('F', 0);
	//::Sleep(40);
	CKeyboard::KeyPress('J', 0);
}