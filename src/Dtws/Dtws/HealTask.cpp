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
		CKeyboard::KeyPress(CKeyboard::Right, 50);
		Sleep(800);
		while (!m_exit)
		{
			//����
			CKeyboard::InputString("5");
			Sleep(3500);
			CKeyboard::KeyPress(CKeyboard::F2);
			Sleep(50);
			CKeyboard::InputString("6");
			Sleep(800);
			CKeyboard::InputString("8");
			Sleep(1400);
			CKeyboard::InputString("7");
			Sleep(800);
			CKeyboard::KeyPress(CKeyboard::F1);
			Sleep(20);
			CKeyboard::InputString("6");
			Sleep(800);
		}
		return;
	}

	int32_t accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		//���������
		CMouse::MoveAbsolute(g_accountPoint[accountIndex], 0);
		CMouse::LeftClick(0);
		Sleep(20);
		CKeyboard::KeyPress(CKeyboard::Right);
	}

	while (!m_exit)
	{
		//����
		int32_t accountIndex = -1;
		while (accountIndex++ != g_accountCount - 1)
		{
			//���������
			CMouse::MoveAbsolute(g_accountPoint[accountIndex], 0);
			CMouse::LeftClick(0);
			Sleep(20);
			CKeyboard::InputString("5");
		}
		Sleep(5000);
		if (m_exit)
		{
			return;
		}

		accountIndex = -1;
		while (accountIndex++ != g_accountCount - 1)
		{
			//���������
			CMouse::MoveAbsolute(g_accountPoint[accountIndex], 0);
			CMouse::LeftClick(0);
			Sleep(20);
			CKeyboard::KeyPress(CKeyboard::F2);
			Sleep(20);
			CKeyboard::InputString("6");
		}
		Sleep(1200);
		if (m_exit)
		{
			return;
		}

		accountIndex = -1;
		while (accountIndex++ != g_accountCount - 1)
		{
			//���������
			CMouse::MoveAbsolute(g_accountPoint[accountIndex], 0);
			CMouse::LeftClick(0);
			Sleep(20);
			CKeyboard::InputString("8");
		}
		Sleep(1200);
		if (m_exit)
		{
			return;
		}

		accountIndex = -1;
		while (accountIndex++ != g_accountCount - 1)
		{
			//���������
			CMouse::MoveAbsolute(g_accountPoint[accountIndex], 0);
			CMouse::LeftClick(0);
			Sleep(20);
			CKeyboard::InputString("7");
		}
		Sleep(1200);
		if (m_exit)
		{
			return;
		}

		accountIndex = -1;
		while (accountIndex++ != g_accountCount - 1)
		{
			//���������
			CMouse::MoveAbsolute(g_accountPoint[accountIndex], 0);
			CMouse::LeftClick(0);
			Sleep(20);
			CKeyboard::KeyPress(CKeyboard::F1);
			Sleep(20);
			CKeyboard::InputString("6");
		}
		Sleep(1200);
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