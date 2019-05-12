#include "stdafx.h"
#include "CRightClickTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

void CRightClickTask::DoTask()
{
	//Sleep(20);
	int32_t count = 3;
	while (count-- != 0)
	{
		CMouse::RightClick(100);
	}
}