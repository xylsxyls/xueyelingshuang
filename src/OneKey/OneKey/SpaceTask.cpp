#include "stdafx.h"
#include "SpaceTask.h"
#include "CKeyBoard/CKeyboardAPI.h"

void SpaceTask::DoTask()
{
	Sleep(20);
	CKeyboard::InputString(" ", 0);
}