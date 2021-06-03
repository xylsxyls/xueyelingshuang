#include "AttackTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

extern xyls::Point g_accountPoint[3];
extern int32_t g_accountCount;

AttackTask::AttackTask():
m_exit(false)
{

}

void AttackTask::DoTask()
{
	if (g_accountCount != 1)
	{
		//点击任务栏
		CMouse::MoveAbsolute(xyls::Point(463, 1061), 0);
		CMouse::LeftClick(0);
	}

	while (!m_exit)
	{
		int32_t accountIndex = -1;
		while (accountIndex++ != g_accountCount - 1)
		{
			if (g_accountCount != 1)
			{
				//点击任务栏
				CMouse::MoveAbsolute(g_accountPoint[accountIndex], 0);
				CMouse::LeftClick(0);
			}
			if (accountIndex == 0)
			{
				Sleep(50);
			}
			Sleep(50);
			CKeyboard::InputString("\t2");
		}
		Sleep(6000);

		accountIndex = -1;
		while (accountIndex++ != g_accountCount - 1)
		{
			if (g_accountCount != 1)
			{
				//点击任务栏
				CMouse::MoveAbsolute(g_accountPoint[accountIndex], 0);
				CMouse::LeftClick(0);
			}
			if (accountIndex == 0)
			{
				Sleep(50);
			}
			Sleep(50);
			CKeyboard::InputString("1");
		}
		Sleep(800);

		accountIndex = -1;
		while (accountIndex++ != g_accountCount - 1)
		{
			if (g_accountCount != 1)
			{
				//点击任务栏
				CMouse::MoveAbsolute(g_accountPoint[accountIndex], 0);
				CMouse::LeftClick(0);
			}
			if (accountIndex == 0)
			{
				Sleep(50);
			}
			Sleep(50);
			CKeyboard::InputString("1");
		}
		Sleep(800);

		accountIndex = -1;
		while (accountIndex++ != g_accountCount - 1)
		{
			if (g_accountCount != 1)
			{
				//点击任务栏
				CMouse::MoveAbsolute(g_accountPoint[accountIndex], 0);
				CMouse::LeftClick(0);
			}
			if (accountIndex == 0)
			{
				Sleep(50);
			}
			Sleep(50);
			CKeyboard::InputString("1");
		}
		Sleep(2000);
	}
}

void AttackTask::StopTask()
{
	m_exit = true;
}

void AttackTask::Sleep(int32_t time)
{
	int32_t count = time / 16;
	while (count-- != 0 && !m_exit)
	{
		::Sleep(16);
	}
}