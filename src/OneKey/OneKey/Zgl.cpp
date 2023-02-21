#include "stdafx.h"
#include "Zgl.h"
#include "CKeyboardConfig/CKeyboardConfigAPI.h"
#include "CqMoreTask.h"
#include "CqfwrTask.h"
#include "Config.h"

void Zgl::mouse()
{

}

void Zgl::keyboard()
{
	if (g_keyboard.m_keyHasDown['W'] && g_keyboard.m_keyDown['Q'] && g_config.m_stopWatch.GetWatchTime() > 1000)
	{
		g_config.m_stopWatch.SetWatchTime(0);
		std::shared_ptr<CqMoreTask> spTask(new CqMoreTask);
		spTask->setParam(true);
		g_config.m_taskThread->PostTask(spTask, 1);
	}
	else if (g_keyboard.m_keyDown['A'] && g_config.m_stopWatch.GetWatchTime() > 1000)
	{
		g_config.m_stopWatch.SetWatchTime(0);
		std::shared_ptr<CqfwrTask> spTask(new CqfwrTask);
		g_config.m_taskThread->PostTask(spTask, 1);
	}
}
