#include "stdafx.h"
#include "Blsy.h"
#include "CKeyboardConfig/CKeyboardConfigAPI.h"
#include "CRMoveTask.h"
#include "CKeyTask.h"
#include "Config.h"
#include "CrTask.h"

void Blsy::mouse()
{

}

void Blsy::keyboard()
{
	if (g_keyboard.m_keyUp['R'] && g_keyboard.m_keyWatch['R'].GetWatchTime() < 200)
	{
		std::shared_ptr<CRMoveTask> spTask(new CRMoveTask);
		g_config.m_taskThread->PostTask(spTask);
	}
	else if (g_config.m_clickMap && g_keyboard.m_keyUp['T'])
	{
		g_config.m_clickMap = false;
		std::shared_ptr<CKeyTask> spTask(new CKeyTask);
		spTask->setParam('M', false);
		g_config.m_taskThread->PostTask(spTask, 1);
	}
	else if (!g_config.m_clickMap && g_keyboard.m_keyUp['T'])
	{
		std::shared_ptr<CrTask> spTask(new CrTask);
		g_config.m_taskThread->PostTask(spTask);
	}
}

void Blsy::timer(int32_t timerId)
{
	if (!g_config.m_clickMap && g_keyboard.m_keyHasDown['T'] && g_keyboard.m_keyWatch['T'].GetWatchTime() > 200)
	{
		g_config.m_clickMap = true;
		std::shared_ptr<CKeyTask> spTask(new CKeyTask);
		spTask->setParam('M', true);
		g_config.m_taskThread->PostTask(spTask, 1);
	}
}