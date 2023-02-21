#include "stdafx.h"
#include "Zy.h"
#include "CKeyboardConfig/CKeyboardConfigAPI.h"
#include "Config.h"
#include "CqrwTask.h"

void Zy::mouse()
{

}

void Zy::keyboard()
{
	if (g_keyboard.m_keyUp['3'] && g_config.m_stopWatch.GetWatchTime() > 500)
	{
		g_config.m_stopWatch.SetWatchTime(0);
		std::shared_ptr<CqrwTask> spTask(new CqrwTask);
		g_config.m_taskThread->PostTask(spTask);
	}
}

void Zy::timer(int32_t timerId)
{

}