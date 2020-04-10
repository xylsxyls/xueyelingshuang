#pragma once
#include "StockStrategy/StockStrategyAPI.h"
#include "StockSolutionMacro.h"

struct StockSolutionAPI ChooseParam
{
	//使用的类型
	StrategyType m_useType;
	//使用的计数类型
	StrategyType m_useCountType;
	//是否是观察类型
	bool m_isObserve;

	ChooseParam()
	{
		m_useType = STRATEGY_INIT;
		m_useCountType = STRATEGY_INIT;
		m_isObserve = false;
	}
};