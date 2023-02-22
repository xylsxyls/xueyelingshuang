#include "FollowTask.h"
#include "Point/PointAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CSystem/CSystemAPI.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "Config.h"

FollowTask::FollowTask():
m_isHeal(false),
m_exit(false)
{

}

void FollowTask::DoTask()
{
	//Sleep(1500);
	//if (g_accountCount != 1)
	//{
	//	//点击任务栏
	//	CMouse::MoveAbsolute(xyls::Point(463, 1061), 0);
	//	CMouse::LeftClick(0);
	//}
	int32_t accountIndex = -1;
	while (accountIndex++ != g_config.m_accountCount - 1)
	{
		if (m_exit)
		{
			return;
		}
		if (g_config.m_accountCount != 1)
		{
			//点击任务栏
			CMouse::MoveAbsolute(g_config.m_clickTop[accountIndex], 0);
			CMouse::MiddleClick();
		}
		if (accountIndex == 0)
		{
			sleep(50);
		}
		int32_t f2Count = 0;
		//跟随
		do 
		{
			if (m_exit)
			{
				return;
			}
			++f2Count;
			if (f2Count > 20)
			{
				break;
			}
			sleep(10);
			CKeyboard::KeyPress(CKeyboard::F2, 0);
		} while (ScreenScript::FindPic(g_config.m_currentExePath + "res\\blood.png", g_config.m_bloodRect[accountIndex]).empty());
	}
	sleep(50);

	accountIndex = -1;
	while (accountIndex++ != g_config.m_accountCount - 1)
	{
		if (m_exit)
		{
			return;
		}
		if (g_config.m_accountCount != 1)
		{
			//点击任务栏
			CMouse::MoveAbsolute(g_config.m_clickTop[accountIndex], 0);
			CMouse::MiddleClick();
		}

		//跟随
		sleep(10);
		CKeyboard::InputString("4", 0);
		sleep(10);
		CKeyboard::InputString("4", 0);
		sleep(10);
		CKeyboard::InputString("4", 0);
	}
	if (m_isHeal)
	{
		CMouse::MoveAbsolute(xyls::Point(396, 1056), 0);
		CMouse::LeftClick(0);
	}
}

void FollowTask::StopTask()
{
	m_exit = true;
}

void FollowTask::setParam(bool isHeal)
{
	m_isHeal = isHeal;
}