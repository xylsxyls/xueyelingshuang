#include "FiveTask.h"
#include "CSystem/CSystemAPI.h"
#include "CKeyboard/CKeyboardAPI.h"

void FiveTask::DoTask()
{
	CSystem::Sleep(950);
	CKeyboard::KeyPress('5');
	CSystem::Sleep(1100);
	CKeyboard::KeyDown('Q');
	CSystem::Sleep(1150);
	CKeyboard::KeyUp('Q');
}