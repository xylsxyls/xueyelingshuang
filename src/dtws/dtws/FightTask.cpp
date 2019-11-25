#include "FightTask.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CScreen/CScreenAPI.h"

FightTask::FightTask():
m_moveTime(0),
m_fightNum(0),
m_skillNum(0)
{

}

void FightTask::DoTask()
{
	RCSend("step = %d", m_step);

	static int f11 = 0;
	if (f11 == 0)
	{
		CKeyboard::KeyPress(CKeyboard::F11);
		f11 = 1;
	}

	CMouse::MoveClick(m_dest, 10);
	Sleep(m_moveTime);
	int32_t x = 0;
	int32_t y = 0;
	int32_t index = -1;
	while (/*index++ <= m_fightNum - 1 ||*/
		!CScreen::FindColor(xyls::Rect(1717, 277, 1886, 312), xyls::Color(51, 204, 51), x, y))
	{
		CKeyboard::InputString("\t", 0);
		CKeyboard::InputString("3", 0);
		int32_t skillIndex = -1;
		while (skillIndex++ != m_skillNum - 1)
		{
			CKeyboard::InputString("1", 0);
			Sleep(1000);
		}
	}
	CMouse::MoveClick(m_over, 10);
	Sleep(m_moveTime);
}

void FightTask::setParam(const xyls::Point& dest, int32_t moveTime, int32_t skillNum, int32_t fightNum, const xyls::Point& over)
{
	m_dest = dest;
	m_moveTime = moveTime;
	m_fightNum = fightNum;
	m_skillNum = skillNum;
	m_over = over;
}