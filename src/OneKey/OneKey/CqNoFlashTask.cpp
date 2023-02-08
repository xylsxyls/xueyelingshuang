#include "stdafx.h"
#include "CqNoFlashTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

CqNoFlashTask::CqNoFlashTask():
m_sleepTime(false)
{

}

void CqNoFlashTask::DoTask()
{
	g_config.m_rightMouse = false;

	/*CKeyboard::KeyDown('N');
	CKeyboard::KeyDown('Q');
	CKeyboard::KeyUp('N');
	CKeyboard::KeyUp('Q');
	Sleep(250);*/
	//Sleep(100);

	//CKeyboard::KeyDown('N');
	//CKeyboard::KeyDown('Q');
	//CKeyboard::KeyUp('N');
	//CKeyboard::KeyUp('Q');
	//CKeyboard::InputString("www", 0);
	//Sleep(50);
	//
	//CKeyboard::InputString("www", 0);
	//Sleep(100);
	////CKeyboard::InputString("www", 0);
	////Sleep(50);
	////CKeyboard::InputString("www", 0);
	////Sleep(50);
	////CKeyboard::InputString("www", 0);
	//CKeyboard::InputString("r", 100);
	//Sleep(50);
	//CKeyboard::InputString("www", 0);
	//Sleep(250);
	//CMouse::RightClick(150);
	//CMouse::RightManyClick(5, 300);
	//return;

	CKeyboard::KeyDown('W');
	CKeyboard::KeyUp('W');
	//CKeyboard::InputString("r", 0);
	Sleep(100);

	//CKeyboard::KeyDown('Q');
	//CKeyboard::KeyUp('Q');
	CKeyboard::KeyDown('D');
	CKeyboard::KeyUp('D');
	CKeyboard::KeyDown('F');
	CKeyboard::KeyUp('F');
	//CKeyboard::KeyDown('Q');
	//CKeyboard::KeyUp('Q');
	
	//Sleep(m_sleepTime);
	//Sleep(50);
	CKeyboard::InputString("rq", 0);
	Sleep(150);
	CMouse::RightClick(0);
	Sleep(50);
	CKeyboard::InputString("rq", 0);
	Sleep(50);
	CMouse::RightClick(0);
	Sleep(300);
	CKeyboard::InputString("rq", 0);

	Sleep(40);
	CMouse::RightClick(0);
	Sleep(40);
	CMouse::RightClick(0);
	Sleep(20);
	CKeyboard::InputString("q", 0);
	CMouse::RightClick(40);
	CKeyboard::InputString("q", 0);
	CMouse::RightClick(150);
	CMouse::RightManyClick(2, 100);
	Sleep(100);
	//CKeyboard::InputString("3", 0);
	Sleep(500);
	CKeyboard::InputString("q", 0);
	CMouse::RightManyClick(6, 200);

	g_config.m_rightMouse = true;
}

//老版250毫秒闪现
void CqNoFlashTask::oldDoTask()
{
	g_config.m_rightMouse = false;

	/*CKeyboard::KeyDown('N');
	CKeyboard::KeyDown('Q');
	CKeyboard::KeyUp('N');
	CKeyboard::KeyUp('Q');
	Sleep(250);*/
	//Sleep(100);

	//CKeyboard::KeyDown('N');
	//CKeyboard::KeyDown('Q');
	//CKeyboard::KeyUp('N');
	//CKeyboard::KeyUp('Q');
	//CKeyboard::InputString("www", 0);
	//Sleep(50);
	//
	//CKeyboard::InputString("www", 0);
	//Sleep(100);
	////CKeyboard::InputString("www", 0);
	////Sleep(50);
	////CKeyboard::InputString("www", 0);
	////Sleep(50);
	////CKeyboard::InputString("www", 0);
	//CKeyboard::InputString("r", 100);
	//Sleep(50);
	//CKeyboard::InputString("www", 0);
	//Sleep(250);
	//CMouse::RightClick(150);
	//CMouse::RightManyClick(5, 300);
	//return;

	CKeyboard::KeyDown('W');
	CKeyboard::KeyUp('W');
	Sleep(250);

	//CKeyboard::KeyDown('Q');
	//CKeyboard::KeyUp('Q');
	CKeyboard::KeyDown('D');
	CKeyboard::KeyUp('D');
	CKeyboard::KeyDown('F');
	CKeyboard::KeyUp('F');
	//CKeyboard::KeyDown('Q');
	//CKeyboard::KeyUp('Q');
	
	//Sleep(m_sleepTime);
	Sleep(50);
	CKeyboard::InputString("rq", 0);

	Sleep(40);
	CMouse::RightClick(0);
	Sleep(40);
	CMouse::RightClick(0);
	Sleep(20);
	CKeyboard::InputString("q", 0);
	CMouse::RightClick(40);
	CKeyboard::InputString("q", 0);
	CMouse::RightClick(150);
	CMouse::RightManyClick(2, 100);
	Sleep(100);
	//CKeyboard::InputString("3", 0);
	Sleep(500);
	CKeyboard::InputString("q", 0);
	CMouse::RightManyClick(6, 200);

	g_config.m_rightMouse = true;
}

void CqNoFlashTask::setParam(int32_t sleepTime)
{
	m_sleepTime = sleepTime;
}