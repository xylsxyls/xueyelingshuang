#include "stdafx.h"
#include "IntoGameTask.h"
#include "CMouse/CMouseAPI.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CStopWatch/CStopWatchAPI.h"
#include "D:\\SendToMessageTest.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "Config.h"

IntoGameTask::IntoGameTask():
m_exit(false),
m_time(0)
{

}

void IntoGameTask::DoTask()
{
	char str[20] = {};
	::GetWindowText(g_config.m_editWnd, str, 20);
	if (str[0] == '2')
	{
		Sleep(10000);
		xyls::Point currentPos = CMouse::GetCurrentPos();
		while (!m_exit)
		{
			CMouse::MoveAbsolute(currentPos, 0);
			CKeyboard::InputString("q");
			Sleep(10000);
		}
	}
	else if (str[0] != 0)
	{
		Sleep(10000);
		xyls::Point currentPos = CMouse::GetCurrentPos();
		while (!m_exit)
		{
			CMouse::MoveAbsolute(currentPos, 0);
			CMouse::RightClick();
			Sleep(3000);
		}
	}
	Sleep(2000);
	std::string path = CSystem::GetCurrentExePath();
	xyls::Point homepagePoint = ScreenScript::FindPic(path + "res\\homepage.png", xyls::Rect(0, 0, 3840, 2160), 0.5).center();
	//RCSend("%d,%d", homepagePoint.x(), homepagePoint.y());
	if (homepagePoint.empty())
	{
		return;
	}
	xyls::Point beginGamePoint = homepagePoint + xyls::Point(152, 512);
	xyls::Point exitGamePoint = homepagePoint + xyls::Point(65, 507);
	xyls::Point intoGamePoint = homepagePoint + xyls::Point(220, 408);
	CMouse::MoveAbsolute(beginGamePoint, 0);
	CMouse::LeftClick(0);

	int32_t findGameCount = 0;
	int32_t allFindTime = 0;

	while (!m_exit)
	{
		CStopWatch findgameTime;
		xyls::Point findgamePoint = ScreenScript::FindPic(path + "res\\findgame.png", xyls::Rect(0, 0, 3840, 2160)).center();
		int32_t findTime = findgameTime.GetWatchTime();
		allFindTime += findTime;
		if (findgamePoint.empty())
		{
			Sleep(1000);
			++findGameCount;
			if (findGameCount * 1000 + allFindTime >= m_time * 1000)
			{
				findGameCount = 0;
				allFindTime = 0;

				CMouse::MoveAbsolute(exitGamePoint, 0);
				CMouse::LeftClick(0);

				Sleep(1000);

				CMouse::MoveAbsolute(beginGamePoint, 0);
				CMouse::LeftClick(0);
			}
			continue;
		}

		CMouse::MoveAbsolute(intoGamePoint, 0);
		CMouse::LeftClick(0);

		int32_t selectHeroCount = 0;
		bool isInGame = false;

		while (!m_exit)
		{
			xyls::Point selectHeroPoint = ScreenScript::FindPic(path + "res\\selecthero.png", xyls::Rect(0, 0, 3840, 2160)).center();
			if (selectHeroPoint.empty())
			{
				Sleep(1000);
				++selectHeroCount;
				if (selectHeroCount == 12)
				{
					break;
				}
				continue;
			}
			isInGame = true;
			break;
		}

		if (!isInGame)
		{
			findGameCount = 0;
			allFindTime = 0;

			CMouse::MoveAbsolute(exitGamePoint, 0);
			CMouse::LeftClick(0);

			Sleep(1000);

			CMouse::MoveAbsolute(beginGamePoint, 0);
			CMouse::LeftClick(0);
		}
		else
		{
			break;
		}
	}

	::MessageBox(NULL, "end", NULL, 0);
}

void IntoGameTask::StopTask()
{
	m_exit = true;
}

void IntoGameTask::setParam(int32_t time)
{
	m_time = time;
}