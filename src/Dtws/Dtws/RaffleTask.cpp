#include "RaffleTask.h"
#include "CSystem/CSystemAPI.h"
#include "CKeyboard/CKeyboardAPI.h"
#include "CMouse/CMouseAPI.h"
#include "DtwsParam.h"
#include "Config.h"
#include "TalkHeroAtomicTask.h"
#include "AssignThreadManager/AssignThreadManagerAPI.h"
#include "ClickWindowAtomicTask.h"

void RaffleTask::DoTask()
{
	if (CSystem::getComputerName() == FIRST_COMPUTER)
	{
		Sleep(g_config.m_goFirstSleepTime);
	}
	int32_t count = 5;
	while (!m_exit && count-- != 0)
	{
		std::shared_ptr<TalkHeroAtomicTask> spTask(new TalkHeroAtomicTask);
		spTask->setAccountIndex(m_accountIndex);
		AssignThreadManager::instance().postTask(spTask);
		Sleep(2000);
		std::shared_ptr<ClickWindowAtomicTask> spClickWindowAtomicTaskTask(new ClickWindowAtomicTask);
		spClickWindowAtomicTaskTask->setAccountIndex(m_accountIndex);
		spClickWindowAtomicTaskTask->accept(true, true);
		spClickWindowAtomicTaskTask->accept(false, false);
		spClickWindowAtomicTaskTask->setParam(g_config.m_getOppositePoint + xyls::Point(0, g_config.m_textLineHeight));
		AssignThreadManager::instance().postTask(spClickWindowAtomicTaskTask);
		Sleep(2000);
	}
}

AssignTask* RaffleTask::copy()
{
	return new RaffleTask;
}