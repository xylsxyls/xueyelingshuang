#include "stdafx.h"
#include "Cwq2nofTask.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "D:\\SendToMessageTest.h"
#include "Config.h"

Cwq2nofTask::Cwq2nofTask():
m_exit(false)
{

}

void Cwq2nofTask::DoTask()
{
	KeyPressE();
	if (Sleep(180))
	{
		return;
	}

	CKeyboard::KeyPress('W', 0);
	if (Sleep(150))
	{
		return;
	}
	KeyPressE();
}

void Cwq2nofTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
}

bool Cwq2nofTask::Sleep(int32_t sleepTime)
{
	return m_sleep.wait(sleepTime);
}

void Cwq2nofTask::KeyPressE()
{
	if (g_config.m_code1 != 0)
	{
		//CKeyboard::KeyPress(code1, 0);
	}
	if (g_config.m_code2 != 0)
	{
		CKeyboard::KeyPress('C', 0);
	}
}

void Cwq2nofTask::KeyPressF()
{
	xyls::Point currentPos = CMouse::GetCurrentPos();
	CMouse::MoveAbsolute(xyls::Point(1224, 957), 0);
	CMouse::LeftClick(0);
	CMouse::MoveAbsolute(currentPos, 0);
}