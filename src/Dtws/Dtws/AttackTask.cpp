#include "AttackTask.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "Config.h"

AttackTask::AttackTask():
m_exit(false)
{

}

void AttackTask::DoTask()
{
	if (g_config.m_accountCount == 1)
	{
		while (!m_exit)
		{
			CKeyboard::InputString("\t2", 0);
			Sleep(3200);
			CKeyboard::InputString("1", 0);
			Sleep(800);
			CKeyboard::InputString("1", 0);
			Sleep(800);
			CKeyboard::InputString("1", 0);
			Sleep(800);
			CKeyboard::InputString("1", 0);
			Sleep(800);
			CKeyboard::InputString("1", 0);
			Sleep(500);
			CKeyboard::InputString("1", 0);
			Sleep(500);
			CKeyboard::InputString("1", 0);
			Sleep(2000);
		}
		return;
	}

	//点击任务栏
	//CMouse::MoveAbsolute(xyls::Point(463, 1061), 0);
	//CMouse::LeftClick(0);

	std::vector<std::pair<std::string, int32_t>> vecSkill;
	vecSkill.push_back(std::pair<std::string, int32_t>("\t2", 3400));
	vecSkill.push_back(std::pair<std::string, int32_t>("1", 800));
	vecSkill.push_back(std::pair<std::string, int32_t>("1", 800));
	vecSkill.push_back(std::pair<std::string, int32_t>("1", 800));
	vecSkill.push_back(std::pair<std::string, int32_t>("1", 800));
	vecSkill.push_back(std::pair<std::string, int32_t>("1", 500));
	vecSkill.push_back(std::pair<std::string, int32_t>("1", 500));
	vecSkill.push_back(std::pair<std::string, int32_t>("1", 2200));

	while (!m_exit)
	{
		int32_t skillIndex = -1;
		while (skillIndex++ != vecSkill.size() - 1)
		{
			int32_t accountIndex = -1;
			while (accountIndex++ != g_config.m_accountCount - 1)
			{
				if (g_config.m_accountCount != 1)
				{
					//点击任务栏
					CMouse::MoveAbsolute(g_config.m_clickTop[accountIndex], 0);
					CMouse::MiddleClick();
				}
				Sleep(50);
				CKeyboard::InputString(vecSkill[skillIndex].first, 0);
			}
			Sleep(vecSkill[skillIndex].second);
		}
	}
}

void AttackTask::StopTask()
{
	m_exit = true;
	m_sleep.signal();
}

void AttackTask::Sleep(int32_t time)
{
	if (m_exit)
	{
		return;
	}
	m_sleep.wait(time);
}