#include "stdafx.h"
#include "CWewTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

void CWewTask::DoTask()
{
	CKeyboard::KeyDown('W');
	CKeyboard::KeyDown('E');
	
	CKeyboard::KeyUp('W');
	CKeyboard::KeyUp('E');

	CKeyboard::KeyDown('W');
	CKeyboard::KeyUp('W');
	CMouse::RightManyClick(6, 60);
}