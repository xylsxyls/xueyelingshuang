#include "stdafx.h"
#include "Blxc.h"
#include "CKeyboardConfig.h"
#include "Config.h"
#include "ATask.h"
#include "CKeyTask.h"
#include "SkillTask.h"
#include "CRightClickTask.h"
#include "CMouseConfig.h"

void Blxc::mouse()
{

}

void Blxc::keyboard()
{
	if (g_keyboard.m_keyUp[SPACE])
	{
		g_config.m_code1 = 0;
		g_config.m_code2 = 'C';
		g_config.m_blxca = false;
		g_config.m_blxcRight = false;
		g_config.m_taskThread->StopAllTask();
	}
	else if (g_keyboard.m_keyDown['W'] && g_mouse.m_rightHasDown)
	{
		g_config.m_blxcRight = true;
	}
	else if (g_keyboard.m_keyUp['W'])
	{
		g_config.m_blxca = true;
		g_config.m_blxcRight = false;
	}
}

void Blxc::timer(int32_t timerId)
{
	static int times = 0;
	++times;

	if (timerId == g_config.m_msTimerId)
	{
		if (g_config.timerMs(100, times))
		{
			if (g_config.m_blxca)
			{
				std::shared_ptr<CKeyTask> spTask(new CKeyTask);
				spTask->setParam('C');
				//g_config.m_taskThread->PostTask(spTask);
			}
			if (g_config.m_blxcRight)
			{
				std::shared_ptr<CRightClickTask> spRightTask(new CRightClickTask);
				spRightTask->setParam(true);
				g_config.m_taskThread->PostTask(spRightTask);
			}
		}
		
		if (g_config.timerMs(500, times))
		{
			std::shared_ptr<SkillTask> spSkillTask(new SkillTask);
			spSkillTask->setParam('9', '8', '7');
			g_config.m_taskThread->PostTask(spSkillTask, 1);
		}
	}
}