#include "stdafx.h"
#include "CwrqTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

void CwrqTask::DoTask()
{
	g_config.m_rightMouse = false;

	CKeyboard::KeyDown('W');
	CKeyboard::KeyUp('W');
	Sleep(300);

	//CMouse::RightClick();
	CKeyboard::KeyDown('R');
	CKeyboard::KeyUp('R');

	//CKeyboard::InputString("r", 0);
	CKeyboard::KeyDown('Q');
	CKeyboard::KeyUp('Q');
	//CMouse::RightClick();
	//Sleep(350);
	//CKeyboard::InputString("r", 0);

	//Sleep(30);
	//CKeyboard::InputString("r", 0);
	//CKeyboard::KeyDown('Q');
	//CKeyboard::KeyUp('Q');
	CMouse::RightClick();
	Sleep(30);

	//CKeyboard::InputString("r", 0);
	//CKeyboard::KeyDown('Q');
	//CKeyboard::KeyUp('Q');
	CMouse::RightClick();
	Sleep(80);

	//CKeyboard::InputString("3", 0);
	//CKeyboard::KeyDown('Q');
	//CKeyboard::KeyUp('Q');
	CMouse::RightClick();
	//CMouse::RightManyClick(2, 100);
	////CKeyboard::InputString("q", 0);
	////CMouse::RightClick(40);
	////CKeyboard::InputString("q", 0);
	//CMouse::RightClick(150);
	//CMouse::RightManyClick(2, 100);
	//Sleep(50);
	//// CKeyboard::InputString("3", 0);
	//Sleep(500);
	////CKeyboard::InputString("q", 0);
	//CMouse::RightManyClick(6, 200);

	g_config.m_rightMouse = true;
}