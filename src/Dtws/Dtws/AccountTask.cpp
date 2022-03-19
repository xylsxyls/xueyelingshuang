#include "AccountTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "CSystem/CSystemAPI.h"
#include "DtwsParam.h"

extern xyls::Point g_accountPoint[3];
extern int32_t g_accountCount;
extern bool g_hook;

void AccountTask::DoTask()
{
	g_hook = false;
	Sleep(2000);
	int32_t accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		Sleep(500);
		CMouse::MoveAbsolute(xyls::Point(39, 38), 50);
		CMouse::LeftClick();
		CMouse::LeftClick();

		Sleep(3000);
		if (g_accountCount != 1)
		{
			CMouse::MoveAbsolute(xyls::Point(984, 651), 50);
			CMouse::LeftClick();
			Sleep(1000);
			CMouse::MoveAbsolute(xyls::Point(984, 651), 50);
			CMouse::LeftClick();
		}

		CMouse::MoveAbsolute(xyls::Point(907, 754), 50);
		int32_t count = 3;
		while (count-- != 0)
		{
			CMouse::LeftClick();
		}
	}
	Sleep(g_accountCount == 1 ? 22000 : 25000);
	std::vector<std::string> accountName;
	if (g_accountCount == 3)
	{
		accountName.push_back("dtws2xiayindao");
		accountName.push_back("dtws2baihuaguxi");
		accountName.push_back("dtws2baihuayixi");
	}
	else if (g_accountCount == 1)
	{
		if (CSystem::getComputerName() == FIRST_COMPUTER)
		{
			accountName.push_back("dtws2shaolin");
		}
		else
		{
			accountName.push_back("dtws2hanbingmen");
		}
	}
	
	accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		if (g_accountCount != 1)
		{
			//点击任务栏
			CMouse::MoveAbsolute(g_accountPoint[accountIndex], 50);
			CMouse::LeftClick();
		}
		//点击账户框
		CMouse::MoveAbsolute(xyls::Point(1004, 473), 50);
		CMouse::LeftClick();
		CKeyboard::KeyPress(CKeyboard::Shift);
		CKeyboard::KeyDown(CKeyboard::Ctrl);
		CKeyboard::KeyDown('A');
		CKeyboard::KeyUp(CKeyboard::Ctrl);
		CKeyboard::KeyUp('A');
		CKeyboard::InputString(accountName[accountIndex]);
		CKeyboard::KeyPress(CKeyboard::Shift);
		CKeyboard::InputString("\tyangnan\n");
	}

	accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		if (g_accountCount != 1)
		{
			//点击任务栏
			CMouse::MoveAbsolute(g_accountPoint[accountIndex], 50);
			CMouse::LeftClick();
		}
		//点击大区
		CMouse::MoveAbsolute(xyls::Point(405, 732), 50);
		CMouse::LeftClick();
		//点击小区
		CMouse::MoveAbsolute(xyls::Point(1381, 350), 50);
		CMouse::LeftClick();
		//开始
		CMouse::MoveAbsolute(xyls::Point(1411, 807), 50);
		CMouse::LeftClick();
	}

	Sleep(3000);

	accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		if (g_accountCount != 1)
		{
			//点击任务栏
			CMouse::MoveAbsolute(g_accountPoint[accountIndex], 50);
			CMouse::LeftClick();
		}
		//点击进入
		CMouse::MoveAbsolute(xyls::Point(1004, 942), 50);
		CMouse::LeftClick();
	}

	Sleep(g_accountCount == 1 ? 20000 : 30000);

	accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		if (g_accountCount != 1)
		{
			//点击任务栏
			CMouse::MoveAbsolute(g_accountPoint[accountIndex], 50);
			CMouse::LeftClick();
		}
		//点击关闭顶部
		CMouse::MoveAbsolute(xyls::Point(921, 47), 50);
		CMouse::LeftClick();
		//点击关闭国家
		CMouse::MoveAbsolute(xyls::Point(1391, 234), 50);
		CMouse::LeftClick();
		//F11
		CKeyboard::KeyPress(CKeyboard::F11);
		//点击关闭中间
		CMouse::MoveAbsolute(xyls::Point(1420, 730), 50);
		CMouse::LeftClick();
	}
	Sleep(500);
	accountIndex = -1;
	while (accountIndex++ != g_accountCount - 1)
	{
		if (g_accountCount != 1)
		{
			//点击任务栏
			CMouse::MoveAbsolute(g_accountPoint[accountIndex], 50);
			CMouse::LeftClick();
		}
		//点击关闭中间
		CMouse::MoveAbsolute(xyls::Point(1398, 306), 50);
		CMouse::LeftClick();
	}
	g_hook = true;
}