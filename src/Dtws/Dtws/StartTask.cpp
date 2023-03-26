#include "StartTask.h"
#include "CMouse/CMouseAPI.h"
#include "DtwsParam.h"
#include "CSystem/CSystemAPI.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "Config.h"

void StartTask::DoTask()
{
	int32_t accountIndex = -1;
	while (accountIndex++ != g_config.m_accountCount - 1)
	{
		Sleep(500);
		CMouse::MoveAbsolute(xyls::Point(39, 38), 50);
		CMouse::LeftManyClick(3);

		Sleep(3000);
		if (CSystem::getComputerName() == SECOND_COMPUTER)
		{
			Sleep(3000);
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
			Sleep(300);
		}

		CMouse::MoveAbsolute(CSystem::getComputerName() == SECOND_COMPUTER ?
			xyls::Point(785, 740) : (CSystem::getComputerName() == FIRST_COMPUTER && g_config.m_screenPixel == 3840 * 2160) ? xyls::Point(1916, 1354) : xyls::Point(968, 812), 50);
		int32_t count = 3;
		while (count-- != 0)
		{
			CMouse::LeftClick();
		}
	}

	if (CSystem::getComputerName() == FIRST_COMPUTER)
	{
		std::string currentExePath = CSystem::GetCurrentExePath();
		ScreenScript::WaitForPic(currentExePath + "res\\titlepic.png", xyls::Rect(0, 0, 50, 50), 0.7, 18000, 300);
	}

	if (CSystem::getComputerName() == SECOND_COMPUTER)
	{
		std::string currentExePath = CSystem::GetCurrentExePath();
		if (!ScreenScript::WaitForPic(currentExePath + "res\\titlepic2.png", xyls::Rect(0, 0, 50, 50), 0.7, 220000, 300))
		{
			CMouse::MoveAbsolute(xyls::Point(666, 388));
			CMouse::LeftClick();
		}
	}
}