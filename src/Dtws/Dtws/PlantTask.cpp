#include "PlantTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "Point/PointAPI.h"
#include "CMouse/CMouseAPI.h"

extern int32_t g_accountCount;
extern xyls::Point g_clickTop[3];

void PlantTask::DoTask()
{
	if (g_accountCount == 1)
	{
		CKeyboard::KeyDown(CKeyboard::Alt);
		CKeyboard::KeyDown('3');
		CKeyboard::KeyUp(CKeyboard::Alt);
		CKeyboard::KeyUp('3');
		return;
	}

	//���������
	//CMouse::MoveAbsolute(xyls::Point(463, 1061), 0);
	//CMouse::LeftClick(0);

	//Sleep(100);
	int32_t accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		//���������
		CMouse::MoveAbsolute(g_clickTop[accountIndex], 0);
		CMouse::MiddleClick();
		sleep(100);
		CKeyboard::KeyDown(CKeyboard::Alt);
		CKeyboard::KeyDown('3');
		CKeyboard::KeyUp(CKeyboard::Alt);
		CKeyboard::KeyUp('3');
	}
}