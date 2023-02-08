#include "stdafx.h"
#include "Nkll.h"
#include "CKeyboardConfig.h"
#include "Config.h"
#include "CRqwTask.h"
#include "CKeyTask.h"
#include "CSystem/CSystemAPI.h"
#include "HeroHeadMsgType.h"
#include "SkillTask.h"
#include "CFindSkillTask.h"

void Nkll::mouse()
{

}

void Nkll::keyboard()
{
	if (g_keyboard.m_keyWatch['R'].GetWatchTime() < 8000)
	{
		char press = 0;
		if (g_keyboard.m_keyDown['1'])
		{
			press = '1';
		}
		else if (g_keyboard.m_keyDown['2'])
		{
			press = '2';
		}
		else if (g_keyboard.m_keyDown['3'])
		{
			press = '3';
		}
		else if (g_keyboard.m_keyDown['4'])
		{
			press = '4';
		}
		else if (g_keyboard.m_keyDown['5'])
		{
			press = '5';
		}
		else if (g_keyboard.m_keyDown['W'])
		{
			press = 'W';
		}
		else if (g_keyboard.m_keyDown['E'])
		{
			press = 'E';
		}
		else if (g_keyboard.m_keyDown['Q'])
		{
			g_keyboard.m_keyWatch['R'].SetWatchTime(9000);
		}
		if (press != 0)
		{
			g_keyboard.m_keyWatch['R'].SetWatchTime(9000);
			std::shared_ptr<CRqwTask> spTask(new CRqwTask);
			spTask->setParam(press, g_config.m_hasS);
			g_config.m_taskThread->PostTask(spTask);
		}
	}
	else if (g_keyboard.m_keyDown['W'])
	{
		std::shared_ptr<CKeyTask> spTask(new CKeyTask);
		spTask->setParam('J', true);
		g_config.m_taskThread->PostTask(spTask, 1);
	}
	else if (g_keyboard.m_keyUp['W'])
	{
		std::shared_ptr<CKeyTask> spTask(new CKeyTask);
		spTask->setParam('J', false);
		g_config.m_taskThread->PostTask(spTask, 1);
	}
	if (g_keyboard.m_keyDown[KEY + '6'])
	{
		::SetWindowTextA(g_config.m_editWnd, "88888");
		std::string path = CSystem::GetCurrentExePath() + "HeroHead\\";

		if (CSystem::DirOrFileExist(path))
		{
			int32_t index = 99;
			while (index++ != 600 - 1)
			{
				CSystem::deleteFile((path + std::to_string(index) + ".jpg").c_str());
			}
		}

		g_config.m_checkHero = true;
		g_config.m_fullLast = true;

		::PostMessage(g_config.m_hWnd, WM_DESTROY_HERO_HEAD, 0, 0);
	}
}

void Nkll::timer(int32_t timerId)
{
	static int32_t skillCount = 0;
	++skillCount;
	if (g_config.timerMs(500, skillCount) && !g_keyboard.m_keyHasDown['T'] && !g_keyboard.m_keyHasDown[TILDE])
	{
		std::shared_ptr<SkillTask> spSkillTask(new SkillTask);
		spSkillTask->setParam('9', '7', '8');
		g_config.m_taskThread->PostTask(spSkillTask, 1);

		std::shared_ptr<CFindSkillTask> spCFindSkillTask(new CFindSkillTask);
		g_config.m_taskThread->PostTask(spCFindSkillTask, 1);
	}

	if (g_config.m_code2 != 'C' && g_config.m_code2Watch.GetWatchTime() > 3000)
	{
		g_config.m_code2 = 'C';
	}
}