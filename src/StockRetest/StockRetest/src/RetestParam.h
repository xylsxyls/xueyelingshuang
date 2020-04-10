#pragma once
#include "StockStrategy/StockStrategyAPI.h"

struct RetestParam
{
	StrategyType m_strategy;
	StrategyType m_strategyCount;
	bool m_isObserve;

	RetestParam()
	{
		m_strategy = STRATEGY_INIT;
		m_strategyCount = STRATEGY_INIT;
		m_isObserve = false;
	}
};