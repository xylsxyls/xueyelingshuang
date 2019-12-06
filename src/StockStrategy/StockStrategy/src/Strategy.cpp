#include "Strategy.h"

Strategy::Strategy():
m_strategyType(STRATEGY_INIT)
{

}

bool Strategy::sell(const IntDateTime& date,
	BigNumber& price,
	BigNumber& percent,
	BigNumber& score,
	const std::shared_ptr<StrategyInfo>& strategyInfo)
{
	return false;
}

StrategyType Strategy::type()
{
	return m_strategyType;
}