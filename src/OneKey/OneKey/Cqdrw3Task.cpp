#include "stdafx.h"
#include "Cqdrw3Task.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

void Cqdrw3Task::DoTask()
{
	CKeyboard::KeyDown('Q');
	CKeyboard::KeyDown('D');

	CKeyboard::KeyUp('Q');
	CKeyboard::KeyUp('D');

	Sleep(350);

	CKeyboard::KeyDown('F');
	CKeyboard::KeyUp('F');

	CKeyboard::KeyDown('R');
	CKeyboard::KeyUp('R');

	CKeyboard::KeyUp('W');
	CKeyboard::KeyDown('W');

	int32_t count = 14;
	while (count-- != 0)
	{
		Sleep(20);
		CMouse::RightClick(0);
		CKeyboard::KeyDown('W');
		CKeyboard::KeyUp('W');
	}
	CMouse::RightClick(20);
	Sleep(80);
	CMouse::RightClick(0);
	Sleep(300);
	CKeyboard::InputString("3", 20);
}