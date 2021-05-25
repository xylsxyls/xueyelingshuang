#include "ATask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CSystem/CSystemAPI.h"

void ATask::DoTask()
{
	CSystem::Sleep(200);
	CKeyboard::KeyDown('3');
	CSystem::Sleep(600);
	CKeyboard::KeyUp('3');
	CKeyboard::KeyDown('E');
	CSystem::Sleep(200);
	CKeyboard::KeyUp('E');
}