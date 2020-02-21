#include "Three5Task.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"

Three5Task::Three5Task():
m_exit(false)
{

}

void Three5Task::DoTask()
{
	while (!m_exit)
	{
		CMouse::MoveClick(xyls::Point(538, 1063), 0);
		Sleep(100);
		CKeyboard::InputString("5");
		Sleep(3500);
		CMouse::MoveClick(xyls::Point(594, 1063), 0);
		Sleep(100);
		CKeyboard::InputString("5");
		Sleep(3500);
		CMouse::MoveClick(xyls::Point(650, 1063), 0);
		Sleep(100);
		CKeyboard::InputString("5");
		Sleep(3500);
	}
}

void Three5Task::StopTask()
{
	m_exit = true;
}