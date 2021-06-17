#include "RiseTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "Point/PointAPI.h"
#include "CMouse/CMouseAPI.h"

extern int32_t g_accountCount;
extern xyls::Point g_clickTop[3];

void RiseTask::DoTask()
{
	if (g_accountCount == 1)
	{
		CKeyboard::KeyDown(CKeyboard::Alt);
		CKeyboard::KeyDown('2');
		CKeyboard::KeyUp(CKeyboard::Alt);
		CKeyboard::KeyUp('2');
		return;
	}

	//点击任务栏
	//CMouse::MoveAbsolute(xyls::Point(463, 1061), 0);
	//CMouse::LeftClick(0);

	//Sleep(100);
	int32_t accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		//点击任务栏
		CMouse::MoveAbsolute(g_clickTop[accountIndex], 0);
		CMouse::MiddleClick();
		Sleep(100);
		CKeyboard::KeyDown(CKeyboard::Alt);
		CKeyboard::KeyDown('2');
		CKeyboard::KeyUp(CKeyboard::Alt);
		CKeyboard::KeyUp('2');
	}
}