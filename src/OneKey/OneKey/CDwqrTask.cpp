#include "stdafx.h"
#include "CDwqrTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

void CDwqrTask::DoTask()
{
	//Sleep(20);
	CKeyboard::InputString("d", 0);
	CKeyboard::InputString("wwwwwww", 20);
	CKeyboard::KeyDown('N');
	CKeyboard::KeyDown('Q');
	CKeyboard::KeyUp('N');
	CKeyboard::KeyUp('Q');
	Sleep(50);
	CKeyboard::KeyDown('N');
	CKeyboard::KeyDown('Q');
	CKeyboard::KeyUp('N');
	CKeyboard::KeyUp('Q');
	Sleep(250);

	CKeyboard::InputString("rrrrrf", 10);
	Sleep(40);
	CMouse::RightManyClick(2, 100);
	//CKeyboard::InputString("q", 0);
	//CMouse::RightClick(40);
	//CKeyboard::InputString("q", 0);
	CKeyboard::InputString("rrrrrf", 10);
	CMouse::RightClick(150);
	CMouse::RightManyClick(2, 100);
	CKeyboard::InputString("qqqqqq", 10);
	CKeyboard::InputString("wwwwww", 10);
	Sleep(50);
	CKeyboard::InputString("3", 0);
	CKeyboard::InputString("rrrrrf", 10);
	Sleep(500);
	CKeyboard::InputString("qqqqqqq", 10);
	CKeyboard::InputString("wwwwwww", 10);
	CMouse::RightManyClick(6, 200);
	Sleep(20);

	return;
}