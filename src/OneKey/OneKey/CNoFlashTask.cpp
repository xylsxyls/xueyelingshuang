#include "stdafx.h"
#include "CNoFlashTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

extern std::atomic<bool> rightMouse;

void CNoFlashTask::DoTask()
{
    rightMouse = false;

	CKeyboard::KeyDown('W');
	CKeyboard::KeyUp('W');
	
	CMouse::RightClick();
	CKeyboard::KeyDown('S');
	CKeyboard::KeyUp('S');
	
	//CKeyboard::InputString("r", 0);
	CKeyboard::KeyDown('Q');
	CKeyboard::KeyUp('Q');
	//CMouse::RightClick();
	Sleep(350);
	CKeyboard::InputString("r", 0);
	
	Sleep(40);
	CKeyboard::KeyDown('Q');
	CKeyboard::KeyUp('Q');
	CMouse::RightClick();
	Sleep(40);
	
	CKeyboard::KeyDown('Q');
	CKeyboard::KeyUp('Q');
	CMouse::RightClick();
	Sleep(40);

	CKeyboard::InputString("3", 0);
	CKeyboard::KeyDown('Q');
	CKeyboard::KeyUp('Q');
	CMouse::RightClick();
	//CMouse::RightManyClick(2, 100);
	////CKeyboard::InputString("q", 0);
	////CMouse::RightClick(40);
	////CKeyboard::InputString("q", 0);
	//CMouse::RightClick(150);
	//CMouse::RightManyClick(2, 100);
    //Sleep(50);
   //// CKeyboard::InputString("3", 0);
	//Sleep(500);
	////CKeyboard::InputString("q", 0);
	//CMouse::RightManyClick(6, 200);

    rightMouse = true;
}