#include "WaterTask.h"
#include "CSystem/CSystemAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "DtwsParam.h"
#include "Config.h"

void WaterTask::DoTask()
{
	Sleep(1500);

	if (CSystem::getComputerName() == THIRD_COMPUTER)
	{
		while (!m_exit)
		{
			int32_t accountIndex = -1;
			while (accountIndex++ != g_config.m_accountCount - 1)
			{
				//µã»÷ÈÎÎñÀ¸
				CMouse::MoveAbsolute(g_config.m_clickTop[accountIndex], 0);
				CMouse::MiddleClick();
				Sleep(500);
				CMouse::MoveAbsolute(g_config.m_talkPoint[accountIndex], 0);
				CMouse::LeftClick(0);
				Sleep(500);
			}
			Sleep(5000);
		}
		
		return;
	}

	while (!m_exit)
	{
		CMouse::MoveAbsolute(xyls::Point(585, 131), 0);
		CMouse::LeftClick(0);
		Sleep(5000);
	}
}