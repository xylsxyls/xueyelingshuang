#include "RunManager.h"
#include "RunTask.h"

RunManager::RunManager() :
m_threadId(0)
{

}

RunManager::~RunManager()
{
	uninit();
}

RunManager& RunManager::instance()
{
	static RunManager s_runManager;
	return s_runManager;
}

void RunManager::init()
{
	m_threadId = CTaskThreadManager::Instance().Init();
}

void RunManager::uninit()
{
	if (m_threadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_threadId);
		m_threadId = 0;
	}
}

void RunManager::simulateRun(uint32_t beginTime, uint32_t endTime, const std::vector<std::string>& stocks,
	int32_t initialFund)
{
	std::shared_ptr<RunTask> spRunTask(new RunTask);
	spRunTask->setParam(beginTime, endTime, stocks, initialFund);
	CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spRunTask);
}