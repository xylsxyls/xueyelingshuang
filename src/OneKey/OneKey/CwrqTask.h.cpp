#include "stdafx.h"
#include "CwrqTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

void CwrqTask::DoTask()
{
	//g_config.m_rightMouse = false;

	CKeyboard::InputString("w", 0);

	Sleep(10);

	//CMouse::RightClick(0);

	Sleep(10);

	CKeyboard::InputString("s", 0);

	Sleep(10);

	CKeyboard::InputString("q", 0);

	Sleep(350);

	CKeyboard::InputString("r", 0);

	Sleep(200);

	CKeyboard::InputString("r", 0);

	//CMouse::RightClick(0);
	Sleep(30);

	//CMouse::RightClick(0);

	//g_config.m_rightMouse = true;
}