#include "FollowTask.h"
#include "Point/PointAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"

extern xyls::Point g_accountPoint[3];
extern int32_t g_accountCount;

void FollowTask::DoTask()
{
	Sleep(1500);
	if (g_accountCount != 1)
	{
		//点击任务栏
		CMouse::MoveAbsolute(xyls::Point(463, 1061), 10);
		CMouse::LeftClick();
	}
	int32_t accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		if (g_accountCount != 1)
		{
			//点击任务栏
			CMouse::MoveAbsolute(g_accountPoint[accountIndex], 50);
			CMouse::LeftClick();
		}
		Sleep(100);
		//跟随
		CKeyboard::KeyPress(CKeyboard::F2);
		Sleep(300);
		CKeyboard::InputString("4");
	}
}