#include "StartTask.h"
#include "CMouse/CMouseAPI.h"
#include "DtwsParam.h"
#include "CSystem/CSystemAPI.h"

extern xyls::Point g_accountPoint[3];
extern int32_t g_accountCount;

void StartTask::DoTask()
{
	int32_t accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		sleep(500);
		CMouse::MoveAbsolute(xyls::Point(39, 38), 50);
		CMouse::LeftClick();
		CMouse::LeftClick();

		sleep(3000);
		if (CSystem::getComputerName() == SECOND_COMPUTER)
		{
			sleep(5000);
		}
		//if (g_accountCount != 1)
		//{
		//	CMouse::MoveAbsolute(xyls::Point(984, 651), 50);
		//	CMouse::LeftClick();
		//	sleep(1000);
		//	CMouse::MoveAbsolute(xyls::Point(984, 651), 50);
		//	CMouse::LeftClick();
		//}

		CMouse::MoveAbsolute(CSystem::getComputerName() == SECOND_COMPUTER ?
			xyls::Point(785, 725) : xyls::Point(968, 812), 50);
		int32_t count = 3;
		while (count-- != 0)
		{
			CMouse::LeftClick();
		}
	}
}