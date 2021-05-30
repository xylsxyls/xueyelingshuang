#include "WaterTask.h"
#include "CSystem/CSystemAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"

extern xyls::Point g_accountPoint[3];
extern int32_t g_accountCount;

WaterTask::WaterTask():
m_exit(false)
{

}

void WaterTask::DoTask()
{
	if (CSystem::getComputerName() == "QQJJQPZPPWSYCVN")
	{
		//点击任务栏
		CMouse::MoveAbsolute(xyls::Point(463, 1061), 0);
		CMouse::LeftClick(0);

		Sleep(100);
		while (!m_exit)
		{
			int32_t accountIndex = -1;
			while (accountIndex++ != g_accountCount - 1)
			{
				//点击任务栏
				CMouse::MoveAbsolute(g_accountPoint[accountIndex], 0);
				CMouse::LeftClick(0);
				Sleep(500);
				CMouse::MoveAbsolute(xyls::Point(585, 131), 0);
				CMouse::LeftClick(0);
				Sleep(500);
			}
			Sleep(6000);
		}
		
		return;
	}

	//点击任务栏
	CMouse::MoveAbsolute(xyls::Point(276, 1059), 0);
	CMouse::LeftClick(0);
	Sleep(500);

	while (!m_exit)
	{
		//点击任务栏
		CMouse::MoveAbsolute(xyls::Point(434, 1062), 0);
		CMouse::LeftClick(0);
		Sleep(500);
		CMouse::MoveAbsolute(xyls::Point(585, 131), 0);
		CMouse::LeftClick(0);
		Sleep(1000);

		CMouse::MoveAbsolute(xyls::Point(603, 1059), 0);
		CMouse::LeftClick(0);
		Sleep(500);
		CMouse::MoveAbsolute(xyls::Point(585, 131), 0);
		CMouse::LeftClick(0);

		Sleep(4000);
	}
}

void WaterTask::StopTask()
{
	m_exit = true;
}

void WaterTask::Sleep(int32_t time)
{
	int32_t count = time / 16;
	while (count-- != 0 && !m_exit)
	{
		::Sleep(16);
	}
}