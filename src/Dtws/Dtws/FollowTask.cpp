#include "FollowTask.h"
#include "Point/PointAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CSystem/CSystemAPI.h"
#include "ScreenScript/ScreenScriptAPI.h"

extern int32_t g_accountCount;
extern xyls::Rect g_bloodRect[3];
extern xyls::Point g_clickTop[3];

FollowTask::FollowTask():
m_isHeal(false)
{

}

void FollowTask::DoTask()
{
	std::string currentExePath = CSystem::GetCurrentExePath();
	//Sleep(1500);
	//if (g_accountCount != 1)
	//{
	//	//点击任务栏
	//	CMouse::MoveAbsolute(xyls::Point(463, 1061), 0);
	//	CMouse::LeftClick(0);
	//}
	int32_t accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		if (g_accountCount != 1)
		{
			//点击任务栏
			CMouse::MoveAbsolute(g_clickTop[accountIndex], 0);
			CMouse::MiddleClick();
		}
		if (accountIndex == 0)
		{
			Sleep(50);
		}
		int32_t f2Count = 0;
		//跟随
		do 
		{
			++f2Count;
			if (f2Count > 20)
			{
				break;
			}
			Sleep(10);
			CKeyboard::KeyPress(CKeyboard::F2, 0);
		} while (ScreenScript::FindPic(currentExePath + "res\\blood.png", g_bloodRect[accountIndex]).empty());
	}
	Sleep(50);

	accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		if (g_accountCount != 1)
		{
			//点击任务栏
			CMouse::MoveAbsolute(g_clickTop[accountIndex], 0);
			CMouse::MiddleClick();
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