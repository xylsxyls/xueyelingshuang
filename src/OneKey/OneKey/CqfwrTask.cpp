#include "stdafx.h"
#include "CqfwrTask.h"
#include "CKeyBoard/CKeyboardAPI.h"

void CqfwrTask::DoTask()
{
	CKeyboard::KeyPress('Q', 0);
	::Sleep(200);
	CKeyboard::KeyPress('F', 0);
	::Sleep(200);
	CKeyboard::KeyPress('W', 0);
	::Sleep(150);
	CKeyboard::KeyPress('R', 0);
}