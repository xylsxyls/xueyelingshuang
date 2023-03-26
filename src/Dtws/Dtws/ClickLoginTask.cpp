#include "ClickLoginTask.h"
#include "Point/PointAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CSystem/CSystemAPI.h"
#include "DtwsParam.h"
#include "Config.h"

void ClickLoginTask::DoTask()
{
	if (g_config.m_accountCount == 3)
	{
		CMouse::MoveAbsolute(xyls::Point(463, 1061), 0);
		CMouse::LeftClick(0);
		Sleep(800);
	}

	int32_t index = -1;
	while (index++ != g_config.m_accountCount - 1)
	{
		if (g_config.m_accountCount == 3)
		{
			CMouse::MoveAbsolute(g_screen.m_accountPoint[index], 0);
			CMouse::LeftClick(0);
			Sleep(500);
		}

		xyls::Point clickPoint = CSystem::getComputerName() == SECOND_COMPUTER ? xyls::Point(810, 206) :
			xyls::Point(1085, 520);
		if (CSystem::getComputerName() == FIRST_COMPUTER && g_config.m_screenPixel == 3840 * 2160)
		{
			clickPoint = xyls::Point(2045, 1597);
		}
		//µã»÷ÕËºÅµÇÂ¼
		CMouse::MoveAbsolute(clickPoint);
		CMouse::LeftClick(200);
		Sleep(500);
		CMouse::MoveAbsolute(clickPoint);
		CMouse::LeftClick(200);
		Sleep(500);
	}
}