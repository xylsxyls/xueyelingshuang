#include "FollowTask.h"
#include "Point/PointAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"

extern xyls::Point g_accountPoint[3];
extern int32_t g_accountCount;

void FollowTask::DoTask()
{
	Sleep(2000);
	int32_t accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		if (g_accountCount != 1)
		{
			//µã»÷ÈÎÎñÀ¸
			CMouse::MoveAbsolute(g_accountPoint[accountIndex], 50);
			CMouse::LeftClick();
		}
		Sleep(200);
		//¸úËæ
		CKeyboard::KeyPress(CKeyboard::F2);
		Sleep(200);
		CKeyboard::InputString("4");
	}
}