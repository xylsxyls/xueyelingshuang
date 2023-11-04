#include "stdafx.h"
#include "CNoFlashTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

void CNoFlashTask::DoTask()
{
	//g_config.m_rightMouse = false;

	CKeyboard::KeyDown('W');
	CKeyboard::KeyUp('W');
	
	CMouse::RightClick(50);
	CKeyboard::KeyDown('S');
	CKeyboard::KeyUp('S');
	
	Sleep(150);
	//CKeyboard::InputString("r", 0);
	//Sleep(150);
	CKeyboard::KeyDown('Q');
	CKeyboard::KeyUp('Q');
	CMouse::RightClick(50);
	Sleep(50);
	CKeyboard::InputString("r", 0);

	CMouse::RightClick(50);
	CKeyboard::KeyDown('S');
	CKeyboard::KeyUp('S');
	//
	Sleep(250);
	////CKeyboard::InputString("r", 0);
	CKeyboard::KeyDown('Q');
	CKeyboard::KeyUp('Q');
	//CMouse::RightClick();
	//Sleep(30);
	//
	////CKeyboard::InputString("r", 0);
	//CKeyboard::KeyDown('Q');
	//CKeyboard::KeyUp('Q');
	//CMouse::RightClick();
	//Sleep(80);
	//
	////CKeyboard::InputString("3", 0);
	//CKeyboard::KeyDown('Q');
	//CKeyboard::KeyUp('Q');
	//CMouse::RightClick();
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

	//g_config.m_rightMouse = true;
}