#include "stdafx.h"
#include "CFlashTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

void CFlashTask::DoTask()
{
	Sleep(50);
	CKeyboard::KeyDown('N');
	CKeyboard::KeyDown('Q');
	CKeyboard::KeyUp('N');
	CKeyboard::KeyUp('Q');
	Sleep(300);
	CKeyboard::InputString("r", 0);
	Sleep(40);
	CMouse::RightManyClick(2, 100);
	CKeyboard::InputString("q", 0);
	CMouse::RightClick(40);
	CKeyboard::InputString("q", 0);
	CMouse::RightClick(150);
	CMouse::RightManyClick(2, 100);
	Sleep(500);
	CKeyboard::InputString("q", 0);
	CMouse::RightManyClick(6, 200);
}