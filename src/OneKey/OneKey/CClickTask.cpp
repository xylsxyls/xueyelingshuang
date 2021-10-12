#include "stdafx.h"
#include "CClickTask.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyBoard/CKeyboardAPI.h"

void CClickTask::DoTask()
{
	CKeyboard::KeyPress('W', 0);
	Sleep(500);
	int32_t count = 30;
	while (count-- != 0)
	{
		CMouse::RightClick(0);
		Sleep(20);
	}
}