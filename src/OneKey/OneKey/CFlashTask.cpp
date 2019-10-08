#include "stdafx.h"
#include "CFlashTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

extern std::atomic<bool> rightMouse;

void CFlashTask::DoTask()
{
	CKeyboard::InputString("www", 0);
	CKeyboard::KeyDown('N');
	CKeyboard::KeyDown('Q');
	CKeyboard::KeyUp('N');
	CKeyboard::KeyUp('Q');
	Sleep(350);
	CKeyboard::InputString("r", 0);
	CMouse::RightManyClick(6, 50);
	Sleep(350);
	Sleep(100);
	CKeyboard::InputString("33333", 0);
	Sleep(20);
	
	return;
    rightMouse = false;

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
    Sleep(50);
    CKeyboard::InputString("3", 0);
	Sleep(500);
	CKeyboard::InputString("q", 0);
	CMouse::RightManyClick(6, 200);

    rightMouse = true;
}