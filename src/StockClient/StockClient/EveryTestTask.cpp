#include "EveryTestTask.h"
#include "StockRetest/StockRetestAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include "StockClient.h"
#include "StockRetest/StockRetestAPI.h"

EveryTestTask::EveryTestTask():
m_stockClient(nullptr),
m_strategyType(STRATEGY_INIT)
{

}

void EveryTestTask::DoTask()
{
	StockEveryRetest stockEveryRetest;
	stockEveryRetest.init(m_strategyType, m_stockClient->m_allFilterStock, m_beginTime, m_endTime, true);
	stockEveryRetest.run();
}

void EveryTestTask::setParam(StrategyType strategyType,
	const IntDateTime& beginTime,
	const IntDateTime& endTime,
	StockClient* stockClient)
{
	m_strategyType = strategyType;
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_stockClient = stockClient;
}