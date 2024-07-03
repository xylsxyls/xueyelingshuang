#include "stdafx.h"
#include "WzCommon.h"
#include "CMouseConfig/CMouseConfigAPI.h"
#include "CKeyboardConfig/CKeyboardConfigAPI.h"
#include "Config.h"
#include "CKeyTask.h"
#include "MoveTask.h"
#include "CMouse/CMouseAPI.h"
#include "HeroTask.h"
#include "SpeakTask.h"

void WzCommon::mouse()
{
	if (g_config.m_moveUse &&
		(g_mouse.m_currentPos.x() < 2 ||
		g_mouse.m_currentPos.x() > g_config.m_screenHeight - 2 ||
		g_mouse.m_currentPos.y() < 2 ||
		g_mouse.m_currentPos.y() > g_config.m_screenHeight - 2) &&
		!g_config.m_hasMove)
	{
		g_config.m_hasMove = true;
		std::shared_ptr<CKeyTask> spTask(new CKeyTask);
		spTask->setParam('V', true);
		//g_config.m_taskThread->PostTask(spTask);
	}
	else if (g_mouse.m_rightUp)
	{
		g_config.m_moveUse = false;
		if (g_config.m_hasMove)
		{
			g_config.m_hasMove = false;
			std::shared_ptr<CKeyTask> spTask(new CKeyTask);
			spTask->setParam('V', false);
			//g_config.m_taskThread->PostTask(spTask);
		}
	}

	if (g_mouse.m_rightWatch.GetWatchTime() < g_config.m_rightClickTime || g_mouse.m_rightHasDown)
	{
		int32_t yTop = (g_config.m_screenHeight - g_config.m_gameScreenHeight) / 2 + g_config.m_gameScreenMargin;
		int32_t yBottom = (g_config.m_screenHeight - g_config.m_gameScreenHeight) / 2 + g_config.m_gameScreenHeight - g_config.m_gameScreenMargin;
		xyls::Point point = CMouse::GetCurrentPos();
		if (point.y() > yBottom)
		{
			point.setY(yBottom);
			std::shared_ptr<MoveTask> spTask(new MoveTask);
			spTask->setParam(point);
			g_config.m_taskThread->PostTask(spTask);
		}
		else if (point.y() < yTop)
		{
			point.setY(yTop);
			std::shared_ptr<MoveTask> spTask(new MoveTask);
			spTask->setParam(point);
			g_config.m_taskThread->PostTask(spTask);
		}
	}
}

void WzCommon::keyboard()
{
	if (g_keyboard.m_keyUp[SPACE])
	{
		if (!(g_config.m_type == 10 && g_keyboard.m_lastKey == 'Q'))
		{
			g_config.m_moveUse = true;
		}
		else if (g_config.m_type == 10)
		{
			std::shared_ptr<CKeyTask> spTask1(new CKeyTask);
			spTask1->setParam('9');
			g_config.m_taskThread->PostTask(spTask1, 1);
			std::shared_ptr<CKeyTask> spTask2(new CKeyTask);
			spTask2->setParam('8');
			g_config.m_taskThread->PostTask(spTask2, 1);
			std::shared_ptr<CKeyTask> spTask3(new CKeyTask);
			spTask3->setParam('7');
			g_config.m_taskThread->PostTask(spTask3, 1);
		}
	}
	if (g_keyboard.m_keyUp[CTRL] && g_keyboard.m_currentKey == CTRL)
	{
		std::shared_ptr<CKeyTask> spTask(new CKeyTask);
		spTask->setParam('P');
		g_config.m_taskThread->PostTask(spTask, 1);
	}
	if (g_keyboard.m_keyHasDown[CTRL] && g_keyboard.m_keyDown['E'])
	{
		if (g_config.m_code1 != 'G')
		{
			g_config.m_codetimes = 2;
			g_config.m_code1 = 'G';
		}
		else if (g_config.m_type == 10)
		{
			g_config.m_code1 = 0;
		}
		else
		{
			g_config.m_code1 = 'H';
		}
	}
	else if (g_keyboard.m_keyDown['E'])
	{
		//if (type == 10)
		//{
		//	static int x = 0;
		//	++x;
		//	if (x % 3 == 0 && code1 != 'G')
		//	{
		//		{
		//			std::shared_ptr<CKeyTask> spTask(new CKeyTask);
		//			spTask->setParam('H', true);
		//			taskThread->PostTask(spTask, 1);
		//		}
		//		{
		//			std::shared_ptr<CKeyTask> spTask(new CKeyTask);
		//			spTask->setParam('H', false);
		//			taskThread->PostTask(spTask, 1);
		//		}
		//		{
		//			std::shared_ptr<CSleepTask> spTask(new CSleepTask);
		//			spTask->setParam(100);
		//			taskThread->PostTask(spTask, 1);
		//		}
		//	}
		//}
		std::shared_ptr<HeroTask> spTask(new HeroTask);
		spTask->setParam(g_config.m_code1, g_config.m_code2);
		g_config.m_taskThread->PostTask(spTask, 2);
	}
	else if (g_keyboard.m_keyDown[KEY + '0'])
	{
		std::shared_ptr<SpeakTask> spTask(new SpeakTask);
		g_config.m_taskThread->PostTask(spTask, 1);
	}
	else if (g_keyboard.m_keyDown[TILDE] && !g_config.m_info)
	{
		g_config.m_info = true;
		std::shared_ptr<CKeyTask> spTask(new CKeyTask);
		spTask->setParam('I');
		g_config.m_taskThread->PostTask(spTask, 1);
	}
	else if (g_keyboard.m_keyUp[TILDE] && g_config.m_info)
	{
		g_config.m_info = false;
		std::shared_ptr<CKeyTask> spTask(new CKeyTask);
		spTask->setParam('I');
		g_config.m_taskThread->PostTask(spTask, 1);
	}

	if (g_config.m_type != 10 && g_config.m_type != 14 && g_config.m_type != 15)
	{
		if (g_keyboard.m_keyDown['W'])
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
	}
	if (g_config.m_type != 9 && g_config.m_type != 10 && g_config.m_type != 13)
	{
		if (g_keyboard.m_keyDown['R'])
		{
			std::shared_ptr<CKeyTask> spTask(new CKeyTask);
			spTask->setParam('X', true);
			g_config.m_taskThread->PostTask(spTask, 1);
		}
		else if (g_keyboard.m_keyUp['R'])
		{
			std::shared_ptr<CKeyTask> spTask(new CKeyTask);
			spTask->setParam('X', false);
			g_config.m_taskThread->PostTask(spTask, 1);
		}
	}
	if (g_config.m_type != 10 && g_config.m_type != 13)
	{
		if (g_keyboard.m_keyDown['T'] && g_config.m_stopWatch.GetWatchTime() > 500)
		{
			g_config.m_stopWatch.SetWatchTime(0);
			std::shared_ptr<CKeyTask> spTask(new CKeyTask);
			spTask->setParam('M', true);
			g_config.m_taskThread->PostTask(spTask, 1);
		}
		else if (g_keyboard.m_keyUp['T'])
		{
			std::shared_ptr<CKeyTask> spTask(new CKeyTask);
			spTask->setParam('M', false);
			g_config.m_taskThread->PostTask(spTask, 1);
		}
	}
}