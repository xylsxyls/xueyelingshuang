#include "stdafx.h"
#include "CSmallFlashTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

void CSmallFlashTask::DoTask()
{
	g_config.m_rightMouse = false;

	//Sleep(20);
	CMouse::RightClick(0);
	Sleep(100);
	CMouse::RightClick(0);
	Sleep(100);
	CMouse::RightClick(0);
	Sleep(50);
	CMouse::RightClick(0);
	CKeyboard::KeyDown('N');
	CKeyboard::KeyDown('Q');
	CKeyboard::KeyUp('N');
	CKeyboard::KeyUp('Q');
	CMouse::RightClick(20);
	CMouse::RightClick(20);
	CMouse::RightClick(20);
	CMouse::RightClick(20);
	CMouse::RightClick(20);
	CMouse::RightClick(20);
	g_config.m_rightMouse = true;
}