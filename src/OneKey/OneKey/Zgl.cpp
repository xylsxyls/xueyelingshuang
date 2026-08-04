#include "stdafx.h"
#include "Zgl.h"
#include "CKeyboardConfig/CKeyboardConfigAPI.h"
#include "CqMoreTask.h"
#include "CqfwrTask.h"
#include "Config.h"
#include "SkillTask.h"
#include "CKeyTask.h"
#include "CSleepTask.h"

void Zgl::mouse()
{

}

void Zgl::keyboard()
{
	//if (g_keyboard.m_keyHasDown['W'] && g_keyboard.m_keyDown['Q'] && g_config.m_stopWatch.GetWatchTime() > 1000)
	//{
	//	g_config.m_stopWatch.SetWatchTime(0);
	//	std::shared_ptr<CqMoreTask> spTask(new CqMoreTask);
	//	spTask->setParam(true);
	//	g_config.m_taskThread->PostTask(spTask, 1);
	//}
	if (g_keyboard.m_keyDown['A'] && g_config.m_stopWatch.GetWatchTime() > 1000)
	{
		//g_config.m_stopWatch.SetWatchTime(0);
		std::shared_ptr<CqfwrTask> spTask(new CqfwrTask);
		g_config.m_taskThread->PostTask(spTask, 1);
	}
	else if (g_keyboard.m_keyDown['R'] && g_config.m_stopWatch.GetWatchTime() > 1000)
	{
		g_config.m_stopWatch.SetWatchTime(0);
		//std::shared_ptr<CSleepTask> spSleepTask(new CSleepTask);
		//spSleepTask->setParam(1500);
		//g_config.m_taskThread->PostTask(spSleepTask, 1);
		std::shared_ptr<CKeyTask> spTask(new CKeyTask);
		spTask->setParam('N');
		g_config.m_taskThread->PostTask(spTask, 1);
	}
	//else if (g_keyboard.m_keyDown['W'] && g_config.m_stopWatch.GetWatchTime() > 1000)
	//{
	//	g_config.m_stopWatch.SetWatchTime(0);
	//	std::shared_ptr<CKeyTask> spTask(new CKeyTask);
	//	spTask->setParam('J', false);
	//	g_config.m_taskThread->PostTask(spTask, 1);
	//}
}

void Zgl::timer(int32_t timerId)
{
	static int times = 0;
	++times;

	if (timerId == g_config.m_msTimerId)
	{
		if (g_config.timerMs(500, times))
		{
			std::shared_ptr<SkillTask> spSkillTask(new SkillTask);
			spSkillTask->setParam('9', '7', '8');
			g_config.m_taskThread->PostTask(spSkillTask, 1);
		}
	}
}