#include "RealTestTask.h"
#include "StockRetest/StockRetestAPI.h"

RealTestTask::RealTestTask():
m_solutionEnum(SOLUTION_INIT),
m_strategyEnum(STRATEGY_INIT),
m_beginTime(0, 0),
m_endTime(0, 0),
m_stockClient(nullptr)
{

}

void RealTestTask::DoTask()
{
	StockRealRetest stockRealRetest;
	stockRealRetest.init(m_solutionEnum, m_strategyEnum, m_beginTime, m_endTime);
	stockRealRetest.load();
	stockRealRetest.run();
}

void RealTestTask::setParam(SolutionEnum solutionEnum,
	StrategyEnum strategyEnum,
	const IntDateTime& beginTime,
	const IntDateTime& endTime,
	StockClient* stockClient)
{
	m_solutionEnum = solutionEnum;
	m_strategyEnum = strategyEnum;
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_stockClient = stockClient;
}