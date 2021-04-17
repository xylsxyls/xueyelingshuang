#include "HealTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

extern xyls::Point g_accountPoint[3];
extern int32_t g_accountCount;

HealTask::HealTask():
m_exit(false)
{

}

void HealTask::DoTask()
{
	Sleep(1000);
	while (!m_exit)
	{
		int32_t accountIndex = -1;
		while (accountIndex++ != g_accountCount - 1)
		{
			if (g_accountCount != 1)
			{
				//µã»÷ÈÎÎñÀ¸
				CMouse::MoveAbsolute(g_accountPoint[accountIndex], 50);
				CMouse::LeftClick();
			}
			Sleep(200);
			//²¹³ä
			CKeyboard::InputString("5");
			Sleep(2000);
		}
		Sleep(3500);
	}
}

void HealTask::StopTask()
{
	m_exit = true;
}

void HealTask::Sleep(int32_t time)
{
	int32_t count = time / 10;
	while (count-- != 0 && !m_exit)
	{
		::Sleep(10);
	}
}
