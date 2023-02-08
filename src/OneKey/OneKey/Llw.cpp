#include "stdafx.h"
#include "Llw.h"
#include "CKeyboardConfig.h"
#include "Config.h"
#include "CfwqTask.h"

void Llw::mouse()
{

}

void Llw::keyboard()
{
	if (g_keyboard.m_keyDown['3'] && g_config.m_stopWatch.GetWatchTime() > 2000)
	{
		std::shared_ptr<CfwqTask> spTask(new CfwqTask);
		g_config.m_taskThread->PostTask(spTask);
	}
}