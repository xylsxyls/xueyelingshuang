#include "GoDestTask.h"
#include "GoOpenAtomicTask.h"
#include "GoWriteDestAtomicTask.h"
#include "AcceptAtomicTask.h"
#include "AssignThreadManager/AssignThreadManagerAPI.h"
#include "Config.h"
#include "CSystem/CSystemAPI.h"
#include "DtwsParam.h"

GoDestTask::GoDestTask():
m_clickIndex(0),
m_isClick(false)
{

}

void GoDestTask::DoTask()
{
	if (CSystem::getComputerName() == FIRST_COMPUTER)
	{
		Sleep(g_config.m_goFirstSleepTime);
	}
	{
		std::shared_ptr<GoOpenAtomicTask> spTask;
		do
		{
			spTask.reset(new GoOpenAtomicTask);
			spTask->setAccountIndex(m_accountIndex);
			spTask->go(true, false);
			spTask->go(false, true);
			AssignThreadManager::instance().postTask(spTask);
			spTask->waitForEnd();
		} while (spTask->isReDo() && !m_exit);
	}
	
	{
		std::shared_ptr<GoWriteDestAtomicTask> spTask;
		do
		{
			spTask.reset(new GoWriteDestAtomicTask);
			spTask->setAccountIndex(m_accountIndex);
			spTask->go(true, true);
			spTask->go(false, false);
			spTask->setParam(m_dest, m_clickIndex);
			AssignThreadManager::instance().postTask(spTask);
			spTask->waitForEnd();
		} while (spTask->isReDo() && !m_exit);
	}

	if (m_isClick)
	{
		std::shared_ptr<AcceptAtomicTask> spTask;
		do
		{
			spTask.reset(new AcceptAtomicTask);
			spTask->setAccountIndex(m_accountIndex);
			spTask->accept(true, true);
			spTask->accept(false, false);
			AssignThreadManager::instance().postTask(spTask);
			spTask->waitForEnd();
		} while (spTask->isReDo() && !m_exit);
	}
}

AssignTask* GoDestTask::copy()
{
	return new GoDestTask;
}

void GoDestTask::setParam(const std::string& dest, int32_t clickIndex, bool isClick)
{
	m_dest = dest;
	m_clickIndex = clickIndex;
	m_isClick = isClick;
}