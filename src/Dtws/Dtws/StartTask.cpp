#include "StartTask.h"
#include "CMouse/CMouseAPI.h"
#include "DtwsParam.h"
#include "CSystem/CSystemAPI.h"
#include "ScreenScript/ScreenScriptAPI.h"

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
			sleep(3000);
		}
		//if (g_accountCount != 1)
		//{
		//	CMouse::MoveAbsolute(xyls::Point(984, 651), 50);
		//	CMouse::LeftClick();
		//	sleep(1000);
		//	CMouse::MoveAbsolute(xyls::Point(984, 651), 50);
		//	CMouse::LeftClick();
		//}

		std::string currentExePath = CSystem::GetCurrentExePath();
		while (!ScreenScript::FindPic(currentExePath + "res\\update.png", xyls::Rect(647, 342, 1107, 579)).empty())
		{
			sleep(300);
		}

		CMouse::MoveAbsolute(CSystem::getComputerName() == SECOND_COMPUTER ?
			xyls::Point(785, 740) : xyls::Point(968, 812), 50);
		int32_t count = 3;
		while (count-- != 0)
		{
			CMouse::LeftClick();
		}
	}

	if (CSystem::getComputerName() == FIRST_COMPUTER)
	{
		std::string currentExePath = CSystem::GetCurrentExePath();
		ScreenScript::WaitForPic(currentExePath + "res\\titlepic.png", xyls::Rect(0, 0, 50, 50), 18000, 300);
	}

	if (CSystem::getComputerName() == SECOND_COMPUTER)
	{
		std::string currentExePath = CSystem::GetCurrentExePath();
		if (!ScreenScript::WaitForPic(currentExePath + "res\\titlepic2.png", xyls::Rect(0, 0, 50, 50), 220000, 300))
		{
			CMouse::MoveAbsolute(xyls::Point(666, 388));
			CMouse::LeftClick();
		}
	}
}