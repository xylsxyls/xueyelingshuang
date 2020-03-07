#include "OnceTestTask.h"
#include "StockRetest/StockRetestAPI.h"

OnceTestTask::OnceTestTask() :
m_solutionType(SOLUTION_INIT),
m_beginTime(0, 0),
m_endTime(0, 0),
m_stockClient(nullptr)
{

}

void OnceTestTask::DoTask()
{
	StockOnceRetest stockOnceRetest;
	stockOnceRetest.init(m_solutionType, m_vecStrategyType, m_beginTime, m_endTime, "20000000000", true);
	stockOnceRetest.load();
	stockOnceRetest.run();
}

void OnceTestTask::setParam(SolutionType solutionType,
	std::vector<std::pair<StrategyType, StrategyType>>& vecStrategyType,
	const IntDateTime& beginTime,
	const IntDateTime& endTime,
	StockClient* stockClient)
{
	m_solutionType = solutionType;
	m_vecStrategyType = vecStrategyType;
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_stockClient = stockClient;
}