#include "ConvoyDestTask.h"
#include "CSystem/CSystemAPI.h"
#include "DtwsParam.h"
#include "Config.h"
#include "GoOpenAtomicTask.h"
#include "GoWriteDestAtomicTask.h"
#include "AcceptTwiceAtomicTask.h"
#include "AssignThreadManager/AssignThreadManagerAPI.h"
#include "JiaohumaerAtomicTask.h"

ConvoyDestTask::ConvoyDestTask():
m_clickIndex(0)
{

}

void ConvoyDestTask::DoTask()
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

	{
		std::shared_ptr<AcceptTwiceAtomicTask> spTask;
		do
		{
			{
				std::shared_ptr<JiaohumaerAtomicTask> spTask(new JiaohumaerAtomicTask);
				spTask->setAccountIndex(m_accountIndex);
				spTask->jiaohumaer(true, true);
				spTask->jiaohumaer(false, false);
				AssignThreadManager::instance().postTask(spTask);
				spTask->waitForEnd();
				if (spTask->isFind())
				{
					Sleep(15000);
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
				}
			}
			spTask.reset(new AcceptTwiceAtomicTask);
			spTask->setAccountIndex(m_accountIndex);
			spTask->accept(true, true);
			spTask->accept(false, false);
			AssignThreadManager::instance().postTask(spTask);
			spTask->waitForEnd();
		} while (spTask->isReDo() && !m_exit);
	}
}

AssignTask* ConvoyDestTask::copy()
{
	return new ConvoyDestTask;
}

void ConvoyDestTask::setParam(const std::string& dest, int32_t clickIndex)
{
	m_dest = dest;
	m_clickIndex = clickIndex;
}