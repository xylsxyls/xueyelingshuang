#include "stdafx.h"
#include "Cwq2nofTask.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyBoard/CKeyboardAPI.h"
#include "D:\\SendToMessageTest.h"

extern uint32_t g_threadId;
extern int32_t code1;
extern int32_t code2;
extern bool rightDown;
extern HWND g_editWnd;

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
	if (code1 != 0)
	{
		//CKeyboard::KeyPress(code1, 0);
	}
	if (code2 != 0)
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