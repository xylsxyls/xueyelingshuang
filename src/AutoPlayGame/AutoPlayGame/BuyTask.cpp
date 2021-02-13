#include "BuyTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

void BuyTask::DoTask()
{
	CKeyboard::KeyPress('P');
	CMouse::MoveAbsolute(m_buy1, 100);
	CMouse::LeftDoubleClick();
	CMouse::MoveAbsolute(m_buy2, 100);
	CMouse::LeftDoubleClick();
	CMouse::MoveAbsolute(m_buy3, 100);
	CMouse::LeftDoubleClick();
	CKeyboard::KeyPress('P');
}

void BuyTask::setParam(const xyls::Point buy1, const xyls::Point buy2, const xyls::Point buy3)
{
	m_buy1 = buy1;
	m_buy2 = buy2;
	m_buy3 = buy3;
}