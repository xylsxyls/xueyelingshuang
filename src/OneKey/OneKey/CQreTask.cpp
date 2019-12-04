#include "stdafx.h"
#include "CQreTask.h"
#include "CKeyBoard/CKeyboardAPI.h"

void CQreTask::DoTask()
{
	CKeyboard::KeyDown('N');
	CKeyboard::KeyDown('Q');
	CKeyboard::KeyUp('N');
	CKeyboard::KeyUp('Q');
	Sleep(300);
	CKeyboard::KeyDown('N');
	CKeyboard::KeyDown('R');
	CKeyboard::KeyUp('N');
	CKeyboard::KeyUp('R');
	//Sleep(100);
	CKeyboard::KeyPress('E');
}