#include "WaitTask.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"

void WaitTask::DoTask()
{
	CMouse::MoveAbsolute(m_click, 0);
	CMouse::MiddleClick();
	Sleep(100);

	CKeyboard::KeyPress(CKeyboard::Right);
	CKeyboard::KeyPress(96);
	CKeyboard::KeyPress(96);
	CKeyboard::KeyPress(96);
	CKeyboard::KeyPress(96);
	CKeyboard::KeyPress(96);
	CKeyboard::KeyPress(96);
	CKeyboard::KeyPress(96);
	CKeyboard::KeyPress(96);
}

void WaitTask::setParam(const xyls::Point& click)
{
	m_click = click;
}