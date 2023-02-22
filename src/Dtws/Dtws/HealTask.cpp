#include "HealTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

HealTask::HealTask():
m_exit(false)
{

}

void HealTask::DoTask()
{
	if (g_config.m_accountCount == 1)
	{
		if (g_config.m_muqing)
		{
			CKeyboard::KeyPress(CKeyboard::Right, 20);
			Sleep(300);
			while (!m_exit)
			{
				//补充
				//CKeyboard::KeyPress(CKeyboard::F1);
				//Sleep(20);
				//CKeyboard::InputString("6");
				//Sleep(800);
				CKeyboard::KeyPress(CKeyboard::F2);
				Sleep(150);
				CKeyboard::InputString("6");
				Sleep(2300);
				CKeyboard::InputString("8");
				Sleep(4500);
				CKeyboard::InputString("5");
				Sleep(6500);
			}
		}
		else
		{
			CKeyboard::KeyPress(CKeyboard::Right, 20);
			Sleep(300);
			CKeyboard::KeyDown('8');
			Sleep(800);
			CKeyboard::KeyUp('8');
			Sleep(3500);
			while (!m_exit)
			{
				CKeyboard::KeyPress(CKeyboard::F2);
				Sleep(50);
				CKeyboard::InputString("5");
				Sleep(1600);
			}
		}
		return;
	}

	//点击任务栏
	//CMouse::MoveAbsolute(xyls::Point(463, 1061), 0);
	//CMouse::LeftClick(0);
	//Sleep(100);
	
	int32_t accountIndex = -1;
	while (accountIndex++ != g_config.m_accountCount - 1)
	{
		//点击任务栏
		CMouse::MoveAbsolute(g_config.m_clickTop[accountIndex], 0);
		CMouse::MiddleClick();
		Sleep(100);
		CKeyboard::KeyPress(CKeyboard::Right, 20);
	}

	Sleep(300);

	while (!m_exit)
	{
		//补充
		int32_t accountIndex = -1;
		while (accountIndex++ != g_config.m_accountCount - 1)
		{
			//点击任务栏
			CMouse::MoveAbsolute(g_config.m_clickTop[accountIndex], 0);
			CMouse::MiddleClick();
			Sleep(20);
			CKeyboard::InputString("5");
			Sleep(600);
			CKeyboard::KeyPress(CKeyboard::F2, 0);
			Sleep(1200);
		}
		Sleep(1200);
		if (m_exit)
		{
			return;
		}

		accountIndex = -1;
		while (accountIndex++ != g_config.m_accountCount - 1)
		{
			//点击任务栏
			CMouse::MoveAbsolute(g_config.m_clickTop[accountIndex], 0);
			CMouse::MiddleClick();
			Sleep(50);
			CKeyboard::InputString("6", 0);
			Sleep(1600);
			CKeyboard::InputString("8", 0);
			Sleep(300);
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