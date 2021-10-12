#include "stdafx.h"
#include "CrfTask.h"
#include "CKeyboard/CKeyboardAPI.h"

void CrfTask::DoTask()
{
	CKeyboard::KeyPress('R', 0);
	Sleep(150);
	CKeyboard::KeyPress('F', 0);
}