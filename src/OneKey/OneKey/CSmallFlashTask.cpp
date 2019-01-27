#include "stdafx.h"
#include "CSmallFlashTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

extern std::atomic<bool> rightMouse;

void CSmallFlashTask::DoTask()
{
    rightMouse = false;

	//Sleep(20);
	CMouse::RightClick(0);
	Sleep(100);
	CMouse::RightClick(0);
	Sleep(100);
	CMouse::RightClick(0);
	Sleep(50);
	CMouse::RightClick(0);
	CKeyboard::KeyDown('N');
	CKeyboard::KeyDown('Q');
	CKeyboard::KeyUp('N');
	CKeyboard::KeyUp('Q');
	CMouse::RightClick(20);
	CMouse::RightClick(20);
	CMouse::RightClick(20);
	CMouse::RightClick(20);
	CMouse::RightClick(20);
	CMouse::RightClick(20);
	CKeyboard::KeyDown('3');
	CKeyboard::KeyUp('3');
    rightMouse = true;
}