#include "stdafx.h"
#include "Yxlm.h"
#include "CMouseConfig.h"
#include "CKeyboardConfig.h"
#include "Config.h"
#include "CKeyTask.h"
#include "MoveTask.h"
#include "CMouse/CMouseAPI.h"
#include "CNoFlashTask.h"
#include "CwrqTask.h"

void Yxlm::mouse()
{
	if (g_config.m_moveUse &&
		(g_mouse.m_currentPos.x() < 2 ||
		g_mouse.m_currentPos.x() > 1918 ||
		g_mouse.m_currentPos.y() < 2 ||
		g_mouse.m_currentPos.y() > 1078) &&
		!g_config.m_hasMove)
	{
		g_config.m_hasMove = true;
		std::shared_ptr<CKeyTask> spTask(new CKeyTask);
		spTask->setParam('V', true);
		g_config.m_taskThread->PostTask(spTask);
	}
	else if (g_mouse.m_rightUp)
	{
		g_config.m_moveUse = false;
		if (g_config.m_hasMove)
		{
			g_config.m_hasMove = false;
			std::shared_ptr<CKeyTask> spTask(new CKeyTask);
			spTask->setParam('V', false);
			g_config.m_taskThread->PostTask(spTask);
		}
	}

	if (g_config.m_type == 10 || g_config.m_type == 15)
	{
		//if (g_qKey && keyWatch['Q'].GetWatchTime() < 3500 && leftUp && stopWatch.GetWatchTime() > 2000)
		//{
		//	stopWatch.SetWatchTime(0);
		//	//taskThread->StopAllTask();
		//	std::shared_ptr<Cwq2nofTask> spTask(new Cwq2nofTask);
		//	taskThread->PostTask(spTask, 1);
		//}

		if (g_mouse.m_rightWatch.GetWatchTime() < 2000 || g_mouse.m_rightHasDown)
		{
			xyls::Point point = CMouse::GetCurrentPos();
			if (point.y() > 950)
			{
				point.setY(950);
				std::shared_ptr<MoveTask> spTask(new MoveTask);
				spTask->setParam(point);
				g_config.m_taskThread->PostTask(spTask);
			}
			else if (point.y() < 130)
			{
				point.setY(130);
				std::shared_ptr<MoveTask> spTask(new MoveTask);
				spTask->setParam(point);
				g_config.m_taskThread->PostTask(spTask);
			}
		}
	}
}

void Yxlm::keyboard()
{
	if (g_config.m_type == 1)
	{
		if (g_keyboard.m_keyHasDown['W'] && g_keyboard.m_keyDown['E'] && g_config.m_stopWatch.GetWatchTime() > 3000)
		{
			g_config.m_stopWatch.SetWatchTime(0);
			std::shared_ptr<CNoFlashTask> spTask;
			spTask.reset(new CNoFlashTask);
			g_config.m_taskThread->PostTask(spTask, 1);
		}
		//if (tDown && stopWatch.GetWatchTime() > 500)
		//{
		//	tDown = false;
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CFlashTask> spTask;
		//	spTask.reset(new CFlashTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		else if (g_keyboard.m_keyDown['A'] && g_config.m_stopWatch.GetWatchTime() > 500)
		{
			g_config.m_stopWatch.SetWatchTime(0);
			std::shared_ptr<CwrqTask> spTask;
			spTask.reset(new CwrqTask);
			g_config.m_taskThread->PostTask(spTask, 1);
			//aDown = false;
			//stopWatch.SetWatchTime(0);
			//std::shared_ptr<Cqrw3Task> spTask;
			//spTask.reset(new Cqrw3Task);
			//taskThread->PostTask(spTask, 1);
		}
		//else if (threeDown && stopWatch.GetWatchTime() > 500)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CQreTask> spTask(new CQreTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		else if (g_keyboard.m_keyDown['5'] && g_config.m_stopWatch.GetWatchTime() > 500)
		{
			//if (superWatch.GetWatchTime() < 10000)
			//{
			//	if (openWatch.GetWatchTime() < 500)
			//	{
			//		stopWatch.SetWatchTime(0);
			//		std::shared_ptr<CDwqrTask> spTask;
			//		spTask.reset(new CDwqrTask);
			//		taskThread->PostTask(spTask, 1);
			//	}
			//	else
			//	{
			//		stopWatch.SetWatchTime(0);
			//		std::shared_ptr<Cqdrw3Task> spTask;
			//		spTask.reset(new Cqdrw3Task);
			//		taskThread->PostTask(spTask, 2);
			//	}
			//}
			//else
			//{
			//	if (openWatch.GetWatchTime() < 500)
			//	{
			//		stopWatch.SetWatchTime(0);
			//		std::shared_ptr<CDwqrTask> spTask;
			//		spTask.reset(new CDwqrTask);
			//		taskThread->PostTask(spTask, 1);
			//	}
			//	else
			//	{
			//		stopWatch.SetWatchTime(0);
			//		std::shared_ptr<CqNoFlashTask> spTask;
			//		spTask.reset(new CqNoFlashTask);
			//		spTask->setParam(400);
			//		taskThread->PostTask(spTask, 2);
			//	}
			//}
		}
		//else if (wDown && dDown && stopWatch.GetWatchTime() > 500)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CFlashTask> spTask;
		//	spTask.reset(new CFlashTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		//else if (dDown && stopWatch.GetWatchTime() > 3000)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CSmallFlashTask> spTask;
		//	spTask.reset(new CSmallFlashTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		//else if (wDown && threeDown && w3StopWatch.GetWatchTime() > 3000)
		//{
		//	w3StopWatch.SetWatchTime(0);
		//	std::shared_ptr<CqNoFlashTask> spTask;
		//	spTask.reset(new CqNoFlashTask);
		//	spTask->setParam(290);
		//	taskThread->PostTask(spTask, 2);
		//}
		//else if (cDown && stopWatch.GetWatchTime() > 500)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CqNoFlashTask> spTask;
		//	spTask.reset(new CqNoFlashTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		//else if (vkCodeOpen && wDown && stopWatch.GetWatchTime() > 3000 && openWatch.GetWatchTime() < 3000)
		//{
		//	vkCodeOpen = false;
		//	stopWatch.SetWatchTime(0);
		//	openWatch.SetWatchTime(3000);
		//	std::shared_ptr<CqTask> spTask;
		//	spTask.reset(new CqTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		//else if (wDown && threeWatch.GetWatchTime() < 1500 && stopWatch.GetWatchTime() > 500)
		//{
		//	stopWatch.SetWatchTime(0);
		//	threeWatch.SetWatchTime(3000);
		//	std::shared_ptr<CrqTask> spTask;
		//	spTask.reset(new CrqTask);
		//	taskThread->PostTask(spTask, 1);
		//}

		//else if (qDown && stopWatch.GetWatchTime() > 3000 && superWatch.GetWatchTime() < 10000)
		//{
		//	stopWatch.SetWatchTime(0);
		//	superWatch.SetWatchTime(10000);
		//	std::shared_ptr<Cqrw3Task> spTask;
		//	spTask.reset(new Cqrw3Task);
		//	taskThread->PostTask(spTask, 1);
		//}
	}
	else if (g_config.m_type == 2)
	{
		//if (threeDown && stopWatch.GetWatchTime() > 200)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CWeqTask> spTask;
		//	CWeqTask* weqTask = new CWeqTask;
		//	//(sleepWatch.GetWatchTime()) > 20000 ? 20 : 0
		//	weqTask->setParam(50);
		//	spTask.reset(weqTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		//if (fiveDown && stopWatch.GetWatchTime() > 200)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CWewTask> spTask;
		//	CWewTask* wewTask = new CWewTask;
		//	spTask.reset(wewTask);
		//	taskThread->PostTask(spTask, 1);
		//}
		//else if (rDown)
		//{
		//	sleepWatch.SetWatchTime(0);
		//}
	}
	else if (g_config.m_type == 3)
	{
		//if (threeDown && stopWatch.GetWatchTime() > 500)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CQreTask> spTask(new CQreTask);
		//	taskThread->PostTask(spTask, 1);
		//}
	}
	else if (g_config.m_type == 4)
	{
		//if (wDown && threeDown && stopWatch.GetWatchTime() > 500)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CrFlashTask> spTask(new CrFlashTask);
		//	spTask->setParam(true);
		//	taskThread->PostTask(spTask, 1);
		//}
		//else if (threeDown && stopWatch.GetWatchTime() > 500)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CrFlashTask> spTask(new CrFlashTask);
		//	spTask->setParam(false);
		//	taskThread->PostTask(spTask, 1);
		//}
	}
	else if (g_config.m_type == 5)
	{
		//if (threeDown && stopWatch.GetWatchTime() > 500)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CEwqr1Task> spTask(new CEwqr1Task);
		//	spTask->setParam(false, false);
		//	taskThread->PostTask(spTask, 1);
		//}
		//else if (fiveDown && stopWatch.GetWatchTime() > 500)
		//{
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CEwqr1Task> spTask(new CEwqr1Task);
		//	spTask->setParam(true, true);
		//	taskThread->PostTask(spTask, 1);
		//}
		//else if (aDown && stopWatch.GetWatchTime() > 500)
		//{
		//	aDown = false;
		//	stopWatch.SetWatchTime(0);
		//	std::shared_ptr<CEwqr1Task> spTask(new CEwqr1Task);
		//	spTask->setParam(false, true);
		//	taskThread->PostTask(spTask, 1);
		//}
	}
}