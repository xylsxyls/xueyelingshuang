#include "stdafx.h"
#include "CEwqr1Task.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

CEwqr1Task::CEwqr1Task():
m_isFlash(false),
m_isR(false)
{

}

void CEwqr1Task::DoTask()
{
	CKeyboard::KeyPress('E');
	if (m_isFlash)
	{
		CKeyboard::KeyPress('D');
	}
	CKeyboard::KeyPress('W');
	CKeyboard::KeyPress('Q');
	if (m_isR)
	{
		CKeyboard::KeyPress('1');
		CKeyboard::KeyPress('R');
	}
	if (m_isFlash)
	{
		CKeyboard::KeyPress('F');
	}
}

void CEwqr1Task::setParam(bool isFlash, bool isR)
{
	m_isFlash = isFlash;
	m_isR = isR;
}