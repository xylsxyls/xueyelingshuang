#include "AccountTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CSystem/CSystemAPI.h"
#include "DtwsParam.h"
#include "ScreenScript/ScreenScriptAPI.h"
#include "Config.h"

void AccountTask::DoTask()
{
	g_config.m_hook = false;

	std::vector<std::pair<std::string, std::string>> vecAccount;
	if (g_config.m_accountCount == 3)
	{
		vecAccount.push_back(m_vecAccount[2]);
		vecAccount.push_back(m_vecAccount[3]);
		vecAccount.push_back(m_vecAccount[4]);
	}
	else if (g_config.m_accountCount == 1)
	{
		if (CSystem::getComputerName() == FIRST_COMPUTER)
		{
			vecAccount.push_back(m_vecAccount[0]);
		}
		else
		{
			vecAccount.push_back(m_vecAccount[1]);
		}
	}
	
	int32_t accountIndex = -1;
	while (accountIndex++ != g_config.m_accountCount - 1)
	{
		//点击任务栏
		CMouse::MoveAbsolute(g_screen.m_clickTop[accountIndex], 50);
		CMouse::MiddleClick();

		if (g_config.m_accountCount == 3)
		{
			Sleep(500);
			CMouse::LeftDoubleClick();
			Sleep(1500);
		}
		
		//点击账户框
		CMouse::MoveAbsolute(xyls::Point(585, 131), 50);
		CMouse::LeftClick();
		CKeyboard::InputString("\t");
		Sleep(200);
		CKeyboard::KeyPress(CKeyboard::Shift);
		Sleep(100);
		CKeyboard::KeyDown(CKeyboard::Ctrl);
		CKeyboard::KeyDown('A');
		CKeyboard::KeyUp(CKeyboard::Ctrl);
		CKeyboard::KeyUp('A');
		CKeyboard::InputString(vecAccount[accountIndex].first);
		CKeyboard::KeyPress(CKeyboard::Shift);
		CKeyboard::InputString("\t" + vecAccount[accountIndex].second + "\n", 200);

		CMouse::MoveAbsolute(g_screen.m_clickTop[0], 50);
		CMouse::MiddleClick();

		if (g_config.m_accountCount == 3)
		{
			CMouse::LeftDoubleClick();
			Sleep(1500);
		}
	}

	Sleep(1500);

	accountIndex = -1;
	while (accountIndex++ != g_config.m_accountCount - 1)
	{
		CMouse::MoveAbsolute(g_screen.m_clickTop[accountIndex], 50);
		CMouse::MiddleClick();

		if (g_config.m_accountCount == 3)
		{
			CMouse::LeftDoubleClick();
			Sleep(1500);
		}

		//点击大区
		//CMouse::MoveAbsolute(xyls::Point(405, 732), 50);
		//CMouse::LeftClick();
		////点击小区
		//CMouse::MoveAbsolute(xyls::Point(1381, 350), 50);
		//CMouse::LeftClick();
		//sleep(500);
		//开始
		//CMouse::MoveAbsolute(xyls::Point(1411, 800), 50);

		if (g_config.m_screenPixel == 3840 * 2160)
		{
			CMouse::MoveAbsolute(xyls::Point(2375, 1346), 50);
			CMouse::LeftClick();
		}
		else
		{
			ScreenScript::FindClick("res\\BeginGame.png");
		}
		//CMouse::LeftClick();

		CMouse::MoveAbsolute(g_screen.m_clickTop[0], 50);
		CMouse::MiddleClick();

		if (g_config.m_accountCount == 3)
		{
			CMouse::LeftDoubleClick();
			Sleep(1500);
		}
	}

	Sleep(3000);

	if (CSystem::getComputerName() == SECOND_COMPUTER)
	{
		Sleep(5000);
	}

	accountIndex = -1;
	while (accountIndex++ != g_config.m_accountCount - 1)
	{
		CMouse::MoveAbsolute(g_screen.m_clickTop[accountIndex], 50);
		CMouse::MiddleClick();

		//点击进入
		CMouse::MoveAbsolute(g_screen.m_intoGamePoint[accountIndex], 50);
		CMouse::LeftClick();
	}

	g_config.m_hook = true;
}

void AccountTask::setParam(const std::vector<std::pair<std::string, std::string>>& vecAccount)
{
	m_vecAccount = vecAccount;
}