#include "DaysTestTask.h"
#include "StockRetest/StockRetestAPI.h"

DaysTestTask::DaysTestTask() :
m_solutionType(SOLUTION_INIT),
m_beginTime(0, 0),
m_endTime(0, 0),
m_stockClient(nullptr)
{

}

void DaysTestTask::DoTask()
{
	StockDaysRetest stockDaysRetest;
	stockDaysRetest.init(m_solutionType, m_vecStrategyType, m_beginTime, m_endTime);
	stockDaysRetest.load();
	stockDaysRetest.run();
}

void DaysTestTask::setParam(SolutionType solutionType,
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