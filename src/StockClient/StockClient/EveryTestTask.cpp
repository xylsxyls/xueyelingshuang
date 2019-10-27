#include "EveryTestTask.h"
#include "StockRetest/StockRetestAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include "StockClient.h"
#include "StockRetest/StockRetestAPI.h"

EveryTestTask::EveryTestTask():
m_stockClient(nullptr),
m_strategyEnum(STRATEGY_INIT)
{

}

void EveryTestTask::DoTask()
{
	StockEveryRetest stockEveryRetest;
	stockEveryRetest.init(m_strategyEnum, m_stockClient->m_allFilterStock, m_beginTime, m_endTime, true);
	stockEveryRetest.run();
}

void EveryTestTask::setParam(StrategyEnum strategyEnum,
	const IntDateTime& beginTime,
	const IntDateTime& endTime,
	StockClient* stockClient)
{
	m_strategyEnum = strategyEnum;
	m_beginTime = beginTime;
	m_endTime = endTime;
	m_stockClient = stockClient;
}