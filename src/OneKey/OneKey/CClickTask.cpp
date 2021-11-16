#include "stdafx.h"
#include "CClickTask.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

CClickTask::CClickTask():
m_ctrlCode(0),
m_isSuper(false)
{

}

void CClickTask::DoTask()
{
	xyls::Point currentPos;
	if (m_ctrlCode != 0)
	{
		currentPos = CMouse::GetCurrentPos();
		switch (m_ctrlCode)
		{
		case 1:
		{
			CKeyboard::KeyPress(97, 0);
		}
		break;
		case 2:
		{
			CKeyboard::KeyPress(98, 0);
		}
		break;
		case 3:
		{
			CKeyboard::KeyPress(99, 0);
		}
		break;
		case 4:
		{
			CKeyboard::KeyPress(100, 0);
		}
		break;
		case 5:
		{
			CKeyboard::KeyPress(101, 0);
		}
		break;
		default:
			break;
		}
	}
	if (m_ctrlCode != 0)
	{
		Sleep(220);
		CMouse::MoveAbsolute(xyls::Point(1538, 747), 0);
		CMouse::LeftClick(0);
	}
	else
	{
		CKeyboard::KeyPress('W', 0);
	}
	if (m_ctrlCode != 0)
	{
		CMouse::MoveAbsolute(xyls::Point(currentPos), 0);
	}
	if (m_isSuper)
	{
		Sleep(500);
		int32_t count = 30;
		while (count-- != 0)
		{
			CMouse::RightClick(0);
			Sleep(20);
		}
	}
}

void CClickTask::setParam(int32_t ctrlCode, bool isSuper)
{
	m_ctrlCode = ctrlCode;
	m_isSuper = isSuper;
}