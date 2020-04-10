#include "Strategy.h"

Strategy::Strategy():
m_strategyType(STRATEGY_INIT)
{

}

void Strategy::setStrategyInfo(const std::shared_ptr<StrategyInfo>& strategyInfo)
{
	m_strategyInfo = strategyInfo;
}

bool Strategy::sell(const IntDateTime& date, StockInfo& stockInfo)
{
	return false;
}

bool Strategy::observeSell(const IntDateTime& date, StockInfo& stockInfo)
{
	return false;
}

StrategyType Strategy::type()
{
	return m_strategyType;
}