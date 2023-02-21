#include "stdafx.h"
#include "Hx.h"
#include "CKeyboardConfig/CKeyboardConfigAPI.h"
#include "Config.h"
#include "ChxwTask.h"

void Hx::mouse()
{

}

void Hx::keyboard()
{
	if (g_keyboard.m_keyDown['W'] && g_keyboard.m_keyWatch['W'].GetWatchTime() < 200)
	{
		std::shared_ptr<ChxwTask> spTask(new ChxwTask);
		g_config.m_taskThread->PostTask(spTask);
	}
}
