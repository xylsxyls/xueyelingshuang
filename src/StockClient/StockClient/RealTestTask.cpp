#include "RealTestTask.h"
#include "StockRetest/StockRetestAPI.h"

RealTestTask::RealTestTask():
m_strategyEnum(STRATEGY_INIT),
m_beginTime(0, 0),
m_endTime(0, 0),
m_stockClient(nullptr)
{

}

void RealTestTask::DoTask()
{
	StockRealRetest stockRealRetest;
	stockRealRetest.init(m_strategyEnum, std::vector<std::string>(), m_beginTime, m_endTime);
	stockRealRetest.run();
}

void RealTestTask::setParam(StrategyEnum strategyEnum,
	const IntDateTime& beginTime,
	const IntDateTime& endTime,
	StockClient* stockClient)
{
	m_strategyEnum = strategyEnum;
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_stockClient = stockClient;
}