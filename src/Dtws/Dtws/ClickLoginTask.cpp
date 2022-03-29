#include "ClickLoginTask.h"
#include "Point/PointAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"

extern xyls::Point g_accountPoint[3];
extern int32_t g_accountCount;

void ClickLoginTask::DoTask()
{
	if (g_accountCount == 3)
	{
		CMouse::MoveAbsolute(xyls::Point(463, 1061), 0);
		CMouse::LeftClick(0);
		sleep(800);
	}

	int32_t index = -1;
	while (index++ != g_accountCount - 1)
	{
		if (g_accountCount == 3)
		{
			CMouse::MoveAbsolute(g_accountPoint[index], 0);
			CMouse::LeftClick(0);
			sleep(500);
		}

		//µã»÷ÕËºÅµÇÂ¼
		CMouse::MoveAbsolute(xyls::Point(1085, 389));
		CMouse::LeftClick(200);
		sleep(500);
		CMouse::MoveAbsolute(xyls::Point(1085, 389));
		CMouse::LeftClick(200);
		sleep(500);
	}
}