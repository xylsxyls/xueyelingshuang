#include "SmallTask.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

void SmallTask::DoTask()
{
	if (g_config.m_accountCount != 3)
	{
		return;
	}
	Sleep(1500);

	CMouse::MoveAbsolute(xyls::Point(402, 10), 0);
	CMouse::MiddleClick();

	int32_t count = 3;
	while (count-- != 0)
	{
		CMouse::MoveAbsolute(xyls::Point(402, 10), 0);
		CMouse::LeftDoubleClick();
		Sleep(100);
	}
	
	count = 3;
	while (count-- != 0)
	{
		CMouse::MoveAbsolute(xyls::Point(872, 32), 0);
		CMouse::LeftDown();
		CMouse::MoveAbsolute(xyls::Point(897, 557), 50);
		CMouse::LeftUp();
		Sleep(100);
	}

	CMouse::MoveAbsolute(xyls::Point(463, 1061), 0);
	CMouse::LeftClick(0);
	Sleep(800);

	int32_t index = -1;
	while (index++ != g_config.m_accountCount - 1)
	{
		CMouse::MoveAbsolute(g_screen.m_accountPoint[index], 0);
		CMouse::LeftClick(0);
		Sleep(500);

		if (index == 0)
		{
			CMouse::MoveAbsolute(xyls::Point(817, 300), 0);
			CMouse::LeftDown();
			Sleep(100);
			CMouse::MoveAbsolute(xyls::Point(817, 13), 50);
			CMouse::LeftUp();
			Sleep(100);
		}
		else if (index == 1)
		{
			CMouse::MoveAbsolute(xyls::Point(817, 296), 0);
			CMouse::LeftDown();
			Sleep(100);
			CMouse::MoveAbsolute(xyls::Point(495, 322), 50);
			CMouse::LeftUp();
			Sleep(100);
		}
		else if (index == 2)
		{
			CMouse::MoveAbsolute(xyls::Point(817, 296), 0);
			CMouse::LeftDown();
			Sleep(100);
			CMouse::MoveAbsolute(xyls::Point(1131, 322), 50);
			CMouse::LeftUp();
			Sleep(100);
		}
	}
}