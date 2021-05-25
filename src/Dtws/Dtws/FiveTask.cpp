#include "FiveTask.h"
#include "CSystem/CSystemAPI.h"
#include "CKeyboard/CKeyboardAPI.h"

void FiveTask::DoTask()
{
	CSystem::Sleep(600);
	CKeyboard::KeyPress('5');
	CKeyboard::KeyDown('Q');
	CSystem::Sleep(800);
	CKeyboard::KeyUp('Q');
}