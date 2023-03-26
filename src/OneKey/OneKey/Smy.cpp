#include "stdafx.h"
#include "Smy.h"
#include "CKeyboardConfig/CKeyboardConfigAPI.h"
#include "Config.h"
#include "CSystem/CSystemAPI.h"
#include "CKeyTask.h"
#include "CfwrTask.h"
#include "CwqAllTask.h"
#include "Cwq2Task.h"
#include "CwqTask.h"
#include "CtrlATask.h"
#include "CtrlDTask.h"
#include "CtrlFTask.h"
#include "CwrTask.h"
#include "CrspaceTask.h"
#include "CTabRTask.h"
#include "CEscapeR.h"
#include "HeroHeadMsgType.h"
#include "CMouseConfig/CMouseConfigAPI.h"
#include "CRightClickTask.h"
#include "SkillTask.h"
#include "Common.h"

void Smy::mouse()
{

}

void Smy::keyboard()
{
	if (g_keyboard.m_keyDown[KEY + '6'])
	{
		::SetWindowTextA(g_config.m_editWnd, "88888");
		Common::deleteAllHeroHead(false);
		g_config.m_checkHero = true;
		g_config.m_fullLast = true;
		::PostMessage(g_config.m_hWnd, WM_DESTROY_HERO_HEAD, 0, 0);
	}
	else if (g_keyboard.m_keyDown[KEY + '9'])
	{
		::SetWindowTextA(g_config.m_editWnd, "88888");
		::PostMessage(g_config.m_hWnd, WM_DESTROY_HERO_HEAD, 0, 0);
	}

	if (g_keyboard.m_keyDown['Q'])
	{
		g_config.m_qKey = true;
	}
	else if (g_keyboard.m_keyDown['E'])
	{
		g_config.m_qKey = false;
	}

	if (g_keyboard.m_keyUp[SPACE])
	{
		g_config.m_taskThread->StopAllTask();
		g_config.m_code1 = 0;
		g_config.m_code2 = 'C';
		if (g_keyboard.m_keyWatch['Q'].GetWatchTime() > 3000)
		{
			std::shared_ptr<CKeyTask> spTask(new CKeyTask);
			spTask->setParam('H');
			g_config.m_taskThread->PostTask(spTask, 1);

			std::shared_ptr<CSleepTask> spTask1(new CSleepTask);
			spTask1->setParam(50);
			g_config.m_taskThread->PostTask(spTask1, 1);

			std::shared_ptr<CKeyTask> spTask2(new CKeyTask);
			spTask2->setParam(TAB);
			g_config.m_taskThread->PostTask(spTask2, 1);
		}
		g_mouse.m_rightWatch.SetWatchTime(g_config.m_rightClickTime + 10);
	}

	if (g_config.m_qKey && g_keyboard.m_keyWatch['Q'].GetWatchTime() < 3500)
	{
		int32_t editIndex = -1;
		if (g_keyboard.m_keyDown['1'])
		{
			editIndex = 4;
		}
		else if (g_keyboard.m_keyDown['2'])
		{
			editIndex = 1;
		}
		else if (g_keyboard.m_keyDown['3'])
		{
			editIndex = 0;
			g_config.m_isMain = true;
		}
		else if (g_keyboard.m_keyDown['4'])
		{
			editIndex = 2;
		}
		else if (g_keyboard.m_keyDown['5'])
		{
			editIndex = 3;
		}

		char str[1024] = {};
		::GetWindowTextA(g_config.m_editWnd, str, 1024);
		std::string text = str;

		char realPress = (editIndex == -1 ? '8' : text[editIndex]);

		if (realPress != '8')
		{
			if (g_config.m_code2 == 'C')
			{
				g_config.m_code2Watch.SetWatchTime(0);
			}
			g_config.m_code2 = 48 + realPress;
		}

		if (editIndex != -1)
		{
			if (g_keyboard.m_keyHasDown[CTRL])
			{
				g_config.m_stopWatch.SetWatchTime(0);
				std::shared_ptr<CfwrTask> spTask(new CfwrTask);
				spTask->setParam(editIndex);
				g_config.m_taskThread->PostTask(spTask, 1);
			}
			else
			{
				if (editIndex == 0)
				{
					g_config.m_stopWatch.SetWatchTime(0);
					std::shared_ptr<CwqAllTask> spTask(new CwqAllTask);
					spTask->setParam(true);
					g_config.m_taskThread->PostTask(spTask, 1);
				}
				else
				{
					g_config.m_stopWatch.SetWatchTime(0);
					std::shared_ptr<Cwq2Task> spTask(new Cwq2Task);
					spTask->setParam(editIndex);
					g_config.m_taskThread->PostTask(spTask, 1);
				}
			}
		}
	}
	else if (g_keyboard.m_keyWatch['R'].GetWatchTime() < 5000)
	{
		if (g_keyboard.m_keyDown['3'] || g_keyboard.m_keyDown['2'])
		{
			g_config.m_stopWatch.SetWatchTime(0);
			std::shared_ptr<CwqTask> spTask(new CwqTask);
			spTask->setParam(true, true, 3, false);
			g_config.m_taskThread->PostTask(spTask, 1);
		}
	}
	else if (g_keyboard.m_keyDown['2'] &&
		g_keyboard.m_keyWatch['E'].GetWatchTime() < 5000 &&
		g_config.m_stopWatch.GetWatchTime() > 500)
	{
		g_config.m_stopWatch.SetWatchTime(0);
		std::shared_ptr<CwqTask> spTask(new CwqTask);
		spTask->setParam(true, true, 3, false);
		g_config.m_taskThread->PostTask(spTask, 1);
	}

	if (g_keyboard.m_keyDown['A'] && g_config.m_equipWatch.GetWatchTime() > 1000)
	{
		g_config.m_equipWatch.SetWatchTime(0);
		std::shared_ptr<CtrlATask> spTask(new CtrlATask);
		g_config.m_taskThread->PostTask(spTask, 1);
	}
	else if (g_keyboard.m_keyDown['S'] && g_config.m_equipWatch.GetWatchTime() > 1000)
	{
		g_config.m_equipWatch.SetWatchTime(0);
		std::shared_ptr<CKeyTask> spTask1(new CKeyTask);
		spTask1->setParam('L');
		g_config.m_taskThread->PostTask(spTask1, 1);

		std::shared_ptr<CtrlDTask> spTask(new CtrlDTask);
		g_config.m_taskThread->PostTask(spTask, 1);
		//equipWatch.SetWatchTime(0);
		//std::shared_ptr<CtrlSTask> spTask(new CtrlSTask);
		//taskThread->PostTask(spTask, 1);
		//std::shared_ptr<CtrlDTask> spTask2(new CtrlDTask);
		//taskThread->PostTask(spTask2, 1);
		//std::shared_ptr<CtrlATask> spTask3(new CtrlATask);
		//taskThread->PostTask(spTask3, 1);
	}
	else if (g_keyboard.m_keyDown['S'] && g_config.m_equipWatch.GetWatchTime() > 1000)
	{
		g_config.m_equipWatch.SetWatchTime(0);
		std::shared_ptr<CKeyTask> spTask(new CKeyTask);
		spTask->setParam('L');
		g_config.m_taskThread->PostTask(spTask, 1);
	}
	else if (((g_keyboard.m_keyWatch['S'].GetWatchTime() < 3500) ||
		(g_keyboard.m_keyWatch['A'].GetWatchTime() < 3500) || g_keyboard.m_keyHasDown[CTRL]) &&
		g_keyboard.m_keyDown['D'] &&
		g_config.m_equipWatch.GetWatchTime() > 1000)
	{
		g_config.m_equipWatch.SetWatchTime(0);
		std::shared_ptr<CtrlDTask> spTask(new CtrlDTask);
		g_config.m_taskThread->PostTask(spTask, 1);
	}
	else if (((g_keyboard.m_keyWatch['S'].GetWatchTime() < 3500) ||
		(g_keyboard.m_keyWatch['A'].GetWatchTime() < 3500) || g_keyboard.m_keyHasDown[CTRL]) &&
		g_keyboard.m_keyDown['F'] &&
		g_config.m_equipWatch.GetWatchTime() > 1000)
	{
		g_config.m_equipWatch.SetWatchTime(0);
		std::shared_ptr<CtrlFTask> spTask(new CtrlFTask);
		g_config.m_taskThread->PostTask(spTask, 1);
	}
	else if ((g_keyboard.m_keyHasDown[CTRL] && g_keyboard.m_keyDown['W']) && g_config.m_stopWatch.GetWatchTime() > 500)
	{
		g_config.m_stopWatch.SetWatchTime(0);
		std::shared_ptr<CwrTask> spTask(new CwrTask);
		g_config.m_taskThread->PostTask(spTask, 1);
	}
	else if (g_keyboard.m_keyDown['W'])
	{
		int32_t wCount = 2;
		while (wCount-- != 0)
		{
			std::shared_ptr<CKeyTask> spTask(new CKeyTask);
			spTask->setParam('J');
			g_config.m_taskThread->PostTask(spTask, 1);
			std::shared_ptr<CSleepTask> spSleepTask(new CSleepTask);
			spSleepTask->setParam(100);
			g_config.m_taskThread->PostTask(spSleepTask);
		}
	}
	else if (g_keyboard.m_keyUp['R'])
	{
		if (g_config.m_holdR && g_keyboard.m_keyWatch['R'].GetWatchTime() > 500)
		{
			g_config.m_holdR = false;
			std::shared_ptr<CKeyTask> spTask(new CKeyTask);
			spTask->setParam('X', false);
			g_config.m_taskThread->PostTask(spTask, 1);
		}
		else
		{
			g_config.m_stopWatch.SetWatchTime(0);
			std::shared_ptr<CrspaceTask> spTask(new CrspaceTask);
			g_config.m_taskThread->PostTask(spTask, 1);
		}
	}
	else if (g_keyboard.m_keyDown[TAB] && g_keyboard.m_keyHasDown['R'] && g_config.m_holdR)
	{
		g_config.m_holdR = false;
		std::shared_ptr<CTabRTask> spTask(new CTabRTask);
		g_config.m_taskThread->PostTask(spTask, 3);
	}
	else if (g_keyboard.m_keyUp['T'])
	{
		if (g_config.m_clickMap)
		{
			g_config.m_clickMap = false;
			std::shared_ptr<CKeyTask> spTask(new CKeyTask);
			spTask->setParam('M', false);
			g_config.m_taskThread->PostTask(spTask, 1);
		}
		else
		{
			g_config.m_stopWatch.SetWatchTime(0);
			if (g_keyboard.m_keyWatch['1'].GetWatchTime() > 5000 &&
				g_keyboard.m_keyWatch['2'].GetWatchTime() > 5000 &&
				g_keyboard.m_keyWatch['3'].GetWatchTime() > 5000 &&
				g_keyboard.m_keyWatch['4'].GetWatchTime() > 5000 &&
				g_keyboard.m_keyWatch['5'].GetWatchTime() > 5000)
			{
				std::shared_ptr<CKeyTask> spTask(new CKeyTask);
				spTask->setParam('H');
				g_config.m_taskThread->PostTask(spTask, 1);
				std::shared_ptr<CSleepTask> spSleepTask(new CSleepTask);
				spSleepTask->setParam(100);
				g_config.m_taskThread->PostTask(spSleepTask, 1);
			}
			std::shared_ptr<CEscapeR> spTask(new CEscapeR);
			g_config.m_taskThread->PostTask(spTask);
		}
	}
}

void Smy::timer(int32_t timerId)
{
	if (timerId == g_config.m_msTimerId)
	{
		if (!g_config.m_clickMap && g_keyboard.m_keyHasDown['T'] && g_keyboard.m_keyWatch['T'].GetWatchTime() > 200)
		{
			g_config.m_clickMap = true;
			std::shared_ptr<CKeyTask> spTask(new CKeyTask);
			spTask->setParam('M', true);
			g_config.m_taskThread->PostTask(spTask, 1);
		}
		if (!g_config.m_holdR && g_keyboard.m_keyHasDown['R'] && g_keyboard.m_keyWatch['R'].GetWatchTime() > 200)
		{
			g_config.m_holdR = true;
			std::shared_ptr<CKeyTask> spTask(new CKeyTask);
			spTask->setParam('X', true);
			g_config.m_taskThread->PostTask(spTask, 1);
		}
		//if (g_keyHasDown['S'] && keyWatch['S'].GetWatchTime() > 200)
		//{
		//	std::shared_ptr<CKeyTask> spTask1(new CKeyTask);
		//	spTask1->setParam('L', true);
		//	taskThread->PostTask(spTask1, 1);
		//	std::shared_ptr<CKeyTask> spTask2(new CKeyTask);
		//	spTask2->setParam('L', false);
		//	taskThread->PostTask(spTask2, 1);
		//}
		if (g_keyboard.m_keyHasDown['Q'] && g_keyboard.m_keyWatch['Q'].GetWatchTime() > 200)
		{
			std::shared_ptr<CKeyTask> spTask0(new CKeyTask);
			spTask0->setParam('Q', false);
			g_config.m_taskThread->PostTask(spTask0, 1);
			std::shared_ptr<CKeyTask> spTask1(new CKeyTask);
			spTask1->setParam('K');
			g_config.m_taskThread->PostTask(spTask1, 1);
		}

		if (g_mouse.m_rightHasDown/* && !g_keyHasDown['R']*/)
		{
			static int times = 0;
			++times;
			if (g_config.timerMs(100, times) &&
				(g_keyboard.m_keyWatch['R'].GetWatchTime() < 5000 ||
				g_keyboard.m_keyWatch['W'].GetWatchTime() < 3000 ||
				g_keyboard.m_keyWatch['Q'].GetWatchTime() < 5000))
			{
				std::shared_ptr<CRightClickTask> spTask(new CRightClickTask);
				//spTask->setParam(!g_keyHasDown['R']);
				g_config.m_taskThread->PostTask(spTask);
			}
		}

		static int32_t skillCount = 0;
		++skillCount;
		if (g_config.timerMs(300, skillCount) &&
			g_config.m_code1 == 0 &&
			!g_keyboard.m_keyHasDown['T'] &&
			!g_keyboard.m_keyHasDown[TILDE])
		{
			std::shared_ptr<SkillTask> spSkillTask(new SkillTask);
			spSkillTask->setParam('9', '8', '7');
			g_config.m_taskThread->PostTask(spSkillTask, 1);
		}

		if (g_config.m_code2 != 'C' && g_config.m_code2Watch.GetWatchTime() > 3000)
		{
			g_config.m_code2 = 'C';
			g_config.m_isMain = false;
		}
	}
}