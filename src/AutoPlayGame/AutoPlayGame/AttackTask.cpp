#include "AttackTask.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"

void AttackTask::DoTask()
{
	CKeyboard::KeyPress(code(m_teammate));
	CMouse::MoveAbsolute(xyls::Rect(1920 / 2 - 20, 1080 / 2 - 20, 1920 / 2 + 20, 1080 / 2 + 20));
	CMouse::RightClick();
	CKeyboard::KeyPress('Q');
	Sleep(800);
}

void AttackTask::setParam(const std::string& teammate)
{
	m_teammate = teammate;
}

int32_t AttackTask::code(const std::string& teammate)
{
	if (teammate == "F1")
	{
		return CKeyboard::F1;
	}
	else if (teammate == "F2")
	{
		return CKeyboard::F2;
	}
	else if (teammate == "F3")
	{
		return CKeyboard::F3;
	}
	else if (teammate == "F4")
	{
		return CKeyboard::F4;
	}
	else if (teammate == "F5")
	{
		return CKeyboard::F5;
	}
	return CKeyboard::F5;
}