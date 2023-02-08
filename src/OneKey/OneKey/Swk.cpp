#include "stdafx.h"
#include "Swk.h"
#include "CKeyboardConfig.h"
#include "Config.h"
#include "CerTask.h"

void Swk::mouse()
{

}

void Swk::keyboard()
{
	if (g_keyboard.m_lastKey == 'W' && g_keyboard.m_keyDown['3'] && g_config.m_stopWatch.GetWatchTime() > 2000)
	{
		std::shared_ptr<CerTask> spTask(new CerTask);
		g_config.m_taskThread->PostTask(spTask);
	}
}