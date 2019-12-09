#include "RealTestTask.h"
#include "StockRetest/StockRetestAPI.h"

RealTestTask::RealTestTask():
m_solutionType(SOLUTION_INIT),
m_strategyType(STRATEGY_INIT),
m_beginTime(0, 0),
m_endTime(0, 0),
m_stockClient(nullptr)
{

}

void RealTestTask::DoTask()
{
	StockRealRetest stockRealRetest;
	std::vector<StrategyType> vecStrategyType;
	vecStrategyType.push_back(m_strategyType);
	vecStrategyType.push_back(m_strategyType);
	stockRealRetest.init(m_solutionType, vecStrategyType, m_beginTime, m_endTime, 200000, true);
	stockRealRetest.load();
	stockRealRetest.run();
}

void RealTestTask::setParam(SolutionType solutionType,
	StrategyType strategyType,
	const IntDateTime& beginTime,
	const IntDateTime& endTime,
	StockClient* stockClient)
{
	m_solutionType = solutionType;
	m_strategyType = strategyType;
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_stockClient = stockClient;
}