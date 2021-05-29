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
	if (g_accountCount == 1)
	{
		Sleep(2300);
		CKeyboard::KeyPress(CKeyboard::Right, 0);
		while (!m_exit)
		{
			//补充
			//CKeyboard::KeyPress(CKeyboard::F1);
			//Sleep(20);
			//CKeyboard::InputString("6");
			//Sleep(800);
			CKeyboard::InputString("5");
			Sleep(3600);
			CKeyboard::KeyPress(CKeyboard::F2);
			Sleep(50);
			CKeyboard::InputString("6");
			Sleep(800);
			CKeyboard::InputString("8");
			Sleep(1900);
		}
		return;
	}

	//点击任务栏
	CMouse::MoveAbsolute(xyls::Point(463, 1061), 0);
	CMouse::LeftClick(0);

	Sleep(2300);
	int32_t accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		//点击任务栏
		CMouse::MoveAbsolute(g_accountPoint[accountIndex], 0);
		CMouse::LeftClick(0);
		Sleep(20);
		CKeyboard::KeyPress(CKeyboard::Right, 0);
	}

	while (!m_exit)
	{
		//补充
		int32_t accountIndex = -1;
		while (accountIndex++ != g_accountCount - 1)
		{
			//点击任务栏
			CMouse::MoveAbsolute(g_accountPoint[accountIndex], 0);
			CMouse::LeftClick(0);
			Sleep(20);
			CKeyboard::InputString("5");
			Sleep(600);
			CKeyboard::KeyPress(CKeyboard::F2);
			Sleep(1200);
		}
		Sleep(1200);
		if (m_exit)
		{
			return;
		}

		accountIndex = -1;
		while (accountIndex++ != g_accountCount - 1)
		{
			//点击任务栏
			CMouse::MoveAbsolute(g_accountPoint[accountIndex], 0);
			CMouse::LeftClick(0);
			Sleep(50);
			CKeyboard::InputString("6", 0);
			Sleep(1500);
			CKeyboard::InputString("8", 0);
		}
	}
}

void HealTask::StopTask()
{
	m_exit = true;
}

void HealTask::Sleep(int32_t time)
{
	int32_t count = time / 16;
	while (count-- != 0 && !m_exit)
	{
		::Sleep(16);
	}
}