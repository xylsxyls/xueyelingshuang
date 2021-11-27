#include "stdafx.h"
#include "CrfTask.h"
#include "CKeyboard/CKeyboardAPI.h"

void CrfTask::DoTask()
{
	CKeyboard::KeyPress('X', 0);
	Sleep(300);
	CKeyboard::KeyPress('F', 0);
}