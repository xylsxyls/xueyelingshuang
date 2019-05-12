#include "stdafx.h"
#include "CWeqTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

void CWeqTask::DoTask()
{
	//Sleep(20);
	CKeyboard::KeyDown('W');
	CKeyboard::KeyDown('E');
	CKeyboard::KeyDown('Q');
	CKeyboard::KeyUp('W');
	CKeyboard::KeyUp('E');
	CKeyboard::KeyUp('Q');
	CMouse::RightClick(20);
	CMouse::RightClick(20);
	CMouse::RightClick(20);
	CMouse::RightClick(20);
	CMouse::RightClick(20);
	CMouse::RightClick(20);
}