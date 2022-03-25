#include "AccountTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CSystem/CSystemAPI.h"
#include "DtwsParam.h"

extern xyls::Point g_accountPoint[3];
extern int32_t g_accountCount;
extern xyls::Point g_clickTop[3];
extern xyls::Point g_beginGamePoint[3];
extern xyls::Point g_intoGamePoint[3];
extern bool g_hook;

void AccountTask::DoTask()
{
	g_hook = false;

	std::vector<std::pair<std::string, std::string>> vecAccount;
	if (g_accountCount == 3)
	{
		vecAccount.push_back(m_vecAccount[2]);
		vecAccount.push_back(m_vecAccount[3]);
		vecAccount.push_back(m_vecAccount[4]);
	}
	else if (g_accountCount == 1)
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
	while (accountIndex++ != g_accountCount - 1)
	{
		//点击任务栏
		CMouse::MoveAbsolute(g_clickTop[accountIndex], 50);
		CMouse::MiddleClick();

		//点击账户框
		//CMouse::MoveAbsolute(xyls::Point(1004, 473), 50);
		//CMouse::LeftClick();
		CKeyboard::KeyPress(CKeyboard::Shift);
		CKeyboard::KeyDown(CKeyboard::Ctrl);
		CKeyboard::KeyDown('A');
		CKeyboard::KeyUp(CKeyboard::Ctrl);
		CKeyboard::KeyUp('A');
		CKeyboard::InputString(vecAccount[accountIndex].first);
		CKeyboard::KeyPress(CKeyboard::Shift);
		CKeyboard::InputString("\t" + vecAccount[accountIndex].second + "\n");
	}

	sleep(1500);

	accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		CMouse::MoveAbsolute(g_clickTop[accountIndex], 50);
		CMouse::MiddleClick();

		//点击大区
		//CMouse::MoveAbsolute(xyls::Point(405, 732), 50);
		//CMouse::LeftClick();
		////点击小区
		//CMouse::MoveAbsolute(xyls::Point(1381, 350), 50);
		//CMouse::LeftClick();
		sleep(500);
		//开始
		CMouse::MoveAbsolute(g_accountCount == 1 ? xyls::Point(1411, 807) : g_beginGamePoint[accountIndex], 50);
		CMouse::LeftClick();
	}

	sleep(3000);

	accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		CMouse::MoveAbsolute(g_clickTop[accountIndex], 50);
		CMouse::MiddleClick();

		//点击进入
		CMouse::MoveAbsolute(g_accountCount == 1 ? xyls::Point(1004, 942) : g_intoGamePoint[accountIndex], 50);
		CMouse::LeftClick();
	}

	g_hook = true;
}

void AccountTask::setParam(const std::vector<std::pair<std::string, std::string>>& vecAccount)
{
	m_vecAccount = vecAccount;
}