#include "SmallTask.h"
#include "CMouse/CMouseAPI.h"

extern xyls::Point g_accountPoint[3];
extern int32_t g_accountCount;

void SmallTask::DoTask()
{
	if (g_accountCount != 3)
	{
		return;
	}
	sleep(1500);

	CMouse::MoveAbsolute(xyls::Point(402, 10), 0);
	CMouse::MiddleClick();

	int32_t count = 3;
	while (count-- != 0)
	{
		CMouse::MoveAbsolute(xyls::Point(402, 10), 0);
		CMouse::LeftDoubleClick();
		sleep(100);
	}
	
	count = 3;
	while (count-- != 0)
	{
		CMouse::MoveAbsolute(xyls::Point(872, 32), 0);
		CMouse::LeftDown();
		CMouse::MoveAbsolute(xyls::Point(897, 557));
		CMouse::LeftUp();
		sleep(100);
	}

	CMouse::MoveAbsolute(xyls::Point(463, 1061), 0);
	CMouse::LeftClick(0);
	sleep(800);

	int32_t index = -1;
	while (index++ != g_accountCount - 1)
	{
		CMouse::MoveAbsolute(g_accountPoint[index], 0);
		CMouse::LeftClick(0);
		sleep(500);

		if (index == 0)
		{
			CMouse::MoveAbsolute(xyls::Point(817, 300), 0);
			CMouse::LeftDown();
			sleep(100);
			CMouse::MoveAbsolute(xyls::Point(817, 13));
			CMouse::LeftUp();
			sleep(100);
		}
		else if (index == 1)
		{
			CMouse::MoveAbsolute(xyls::Point(817, 296), 0);
			CMouse::LeftDown();
			sleep(100);
			CMouse::MoveAbsolute(xyls::Point(495, 322));
			CMouse::LeftUp();
			sleep(100);
		}
		else if (index == 2)
		{
			CMouse::MoveAbsolute(xyls::Point(817, 296), 0);
			CMouse::LeftDown();
			sleep(100);
			CMouse::MoveAbsolute(xyls::Point(1131, 322));
			CMouse::LeftUp();
			sleep(100);
		}
	}
}