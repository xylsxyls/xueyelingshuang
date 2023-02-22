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
		sleep(800);
	}

	int32_t index = -1;
	while (index++ != g_config.m_accountCount - 1)
	{
		if (g_config.m_accountCount == 3)
		{
			CMouse::MoveAbsolute(g_config.m_accountPoint[index], 0);
			CMouse::LeftClick(0);
			sleep(500);
		}

		xyls::Point clickPoint = CSystem::getComputerName() == SECOND_COMPUTER ? xyls::Point(810, 206) :
			xyls::Point(1085, 520);
		//µã»÷ÕËºÅµÇÂ¼
		CMouse::MoveAbsolute(clickPoint);
		CMouse::LeftClick(200);
		sleep(500);
		CMouse::MoveAbsolute(clickPoint);
		CMouse::LeftClick(200);
		sleep(500);
	}
}