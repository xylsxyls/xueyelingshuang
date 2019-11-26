#include "Strategy.h"

Strategy::Strategy():
m_buyPollSize(0),
m_strategyType(STRATEGY_INIT)
{

}

int32_t Strategy::buyPollSize()
{
	return m_buyPollSize;
}

StrategyType Strategy::type()
{
	return m_strategyType;
}