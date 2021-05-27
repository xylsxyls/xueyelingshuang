#include "ATask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CSystem/CSystemAPI.h"

void ATask::DoTask()
{
	CSystem::Sleep(1150);
	CKeyboard::KeyDown('3');
	CSystem::Sleep(700);
	CKeyboard::KeyUp('3');
	CSystem::Sleep(1180);
	CKeyboard::KeyPress('E');
	CSystem::Sleep(1050);
	CKeyboard::KeyPress('2');
	Sleep(10);
	CKeyboard::KeyPress('2');
	Sleep(10);
	CKeyboard::KeyPress('2');
	Sleep(10);
	CKeyboard::KeyPress('2');
	Sleep(10);
	CKeyboard::KeyPress('2');
	Sleep(10);
	Sleep(1500);
	CKeyboard::KeyPress('5');
	CSystem::Sleep(1050);
	CKeyboard::KeyPress('5');
	CSystem::Sleep(1100);
	CKeyboard::KeyDown('Q');
	CSystem::Sleep(1150);
	CKeyboard::KeyUp('Q');
	//CSystem::Sleep(700);
	//CKeyboard::KeyUp('E');
}