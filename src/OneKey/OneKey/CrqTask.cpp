#include "stdafx.h"
#include "CrqTask.h"
#include "CKeyBoard/CKeyboardAPI.h"

void CrqTask::DoTask()
{
	//Sleep(300);

	CKeyboard::InputString("q", 0);
}