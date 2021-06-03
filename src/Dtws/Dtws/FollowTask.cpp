#include "FollowTask.h"
#include "Point/PointAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"

extern xyls::Point g_accountPoint[3];
extern int32_t g_accountCount;

FollowTask::FollowTask():
m_isHeal(false)
{

}

void FollowTask::DoTask()
{
	//Sleep(1500);
	if (g_accountCount != 1)
	{
		//点击任务栏
		CMouse::MoveAbsolute(xyls::Point(463, 1061), 0);
		CMouse::LeftClick(0);
	}
	int32_t accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		if (g_accountCount != 1)
		{
			//点击任务栏
			CMouse::MoveAbsolute(g_accountPoint[accountIndex], 0);
			CMouse::LeftClick(0);
		}
		if (accountIndex == 0)
		{
			Sleep(50);
		}
		Sleep(50);
		//跟随
		CKeyboard::KeyPress(CKeyboard::F2, 0);
		Sleep(10);
		CKeyboard::KeyPress(CKeyboard::F2, 0);
		Sleep(10);
		CKeyboard::KeyPress(CKeyboard::F2, 0);
	}
	Sleep(50);

	accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		if (g_accountCount != 1)
		{
			//点击任务栏
			CMouse::MoveAbsolute(g_accountPoint[accountIndex], 0);
			CMouse::LeftClick(0);
		}
		Sleep(10);
		//跟随
		CKeyboard::InputString("444");
	}
	if (m_isHeal)
	{
		CMouse::MoveAbsolute(xyls::Point(396, 1056), 0);
		CMouse::LeftClick();
	}
}

void FollowTask::setParam(bool isHeal)
{
	m_isHeal = isHeal;
}