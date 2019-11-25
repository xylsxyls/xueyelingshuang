#include "stdafx.h"
#include "CRightClickTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

void CRightClickTask::DoTask()
{
	//Sleep(20);
	int32_t count = 30;
	while (count-- != 0)
	{
		CMouse::RightClick(0);
		Sleep(50);
	}
}