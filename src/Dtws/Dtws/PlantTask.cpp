#include "PlantTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "Point/PointAPI.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

void PlantTask::DoTask()
{
	if (g_config.m_accountCount == 1)
	{
		CKeyboard::KeyDown(CKeyboard::Alt);
		CKeyboard::KeyDown('3');
		CKeyboard::KeyUp(CKeyboard::Alt);
		CKeyboard::KeyUp('3');
		return;
	}

	//点击任务栏
	//CMouse::MoveAbsolute(xyls::Point(463, 1061), 0);
	//CMouse::LeftClick(0);

	//Sleep(100);
	int32_t accountIndex = -1;
	while (accountIndex++ != g_config.m_accountCount - 1)
	{
		//点击任务栏
		CMouse::MoveAbsolute(g_config.m_clickTop[accountIndex], 0);
		CMouse::MiddleClick();
		sleep(100);
		CKeyboard::KeyDown(CKeyboard::Alt);
		CKeyboard::KeyDown('3');
		CKeyboard::KeyUp(CKeyboard::Alt);
		CKeyboard::KeyUp('3');
	}
}