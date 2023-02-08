#include "stdafx.h"
#include "Zk.h"
#include "CKeyboardConfig.h"
#include "Config.h"
#include "CClickTask.h"
#include "CrfTask.h"

void Zk::mouse()
{

}

void Zk::keyboard()
{
	if (g_keyboard.m_keyHasDown[CTRL] && g_keyboard.m_keyDown['W'] && g_config.m_stopWatch.GetWatchTime() > 1000)
	{
		g_config.m_stopWatch.SetWatchTime(0);
		std::shared_ptr<CClickTask> spTask(new CClickTask);
		spTask->setParam(0, true);
		g_config.m_taskThread->PostTask(spTask, 1);
	}
	else if (g_keyboard.m_keyDown['R'] && g_config.m_stopWatch.GetWatchTime() > 500)
	{
		g_config.m_stopWatch.SetWatchTime(0);
		std::shared_ptr<CrfTask> spTask(new CrfTask);
		g_config.m_taskThread->PostTask(spTask, 1);
	}
}
