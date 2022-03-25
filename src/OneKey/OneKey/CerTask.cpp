#include "stdafx.h"
#include "CerTask.h"
#include "CKeyboard/CKeyboardAPI.h"

void CerTask::DoTask()
{
	CKeyboard::KeyPress('C', 0);
	Sleep(450);
	CKeyboard::KeyPress('X', 0);
	Sleep(50);
	CKeyboard::KeyPress('X', 0);
	Sleep(50);
	CKeyboard::KeyPress('X', 0);
	Sleep(50);
	CKeyboard::KeyPress('X', 0);
	Sleep(50);
	CKeyboard::KeyPress('X', 0);
	Sleep(50);
	CKeyboard::KeyPress('C', 0);
	Sleep(50);
	CKeyboard::KeyPress('C', 0);
	Sleep(50);
	CKeyboard::KeyPress('C', 0);
	Sleep(50);
	CKeyboard::KeyPress('C', 0);
	Sleep(1000);
	CKeyboard::KeyPress('Q', 0);
}
