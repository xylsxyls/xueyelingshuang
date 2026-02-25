#include "StrategyResultTask.h"
#include "Semaphore/SemaphoreAPI.h"
#include "Strategy.h"

StrategyResultTask::StrategyResultTask() :
m_isComplete(nullptr),
m_exit(false)
{

}

void StrategyResultTask::DoTask()
{
	while (!m_exit && !(*m_isComplete))
	{
		m_resultSemaphore->wait();
		while (!m_exit && !m_resultQueue->empty())
		{
			std::shared_ptr<StrategyResult> strategyResult;
			m_resultQueue->pop(&strategyResult);
			(*m_resultMap)[strategyResult->m_totalReturn].push_back(strategyResult);
		}
	}
}

void StrategyResultTask::StopTask()
{
	m_exit = true;
	m_resultSemaphore->signal();
}

void StrategyResultTask::setParam(LockFreeQueue<std::shared_ptr<StrategyResult>>* resultQueue,
	Semaphore* resultSemaphore, std::map<int32_t, std::vector<std::shared_ptr<StrategyResult>>>* resultMap,
	std::atomic<bool>* isComplete)
{
	m_resultQueue = resultQueue;
	m_resultSemaphore = resultSemaphore;
	m_resultMap = resultMap;
	m_isComplete = isComplete;
}