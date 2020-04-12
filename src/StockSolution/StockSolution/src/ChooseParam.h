#pragma once
#include "StockStrategy/StockStrategyAPI.h"
#include "StockSolutionMacro.h"

enum SolutionType
{
	SOLUTION_INIT,

	AVG_FUND_HIGH_SCORE,

	STRATEGY_SET,

	DISPOSABLE_STRATEGY,

	INTEGRATED_STRATEGY,

	OBSERVE_STRATEGY
};

struct StockSolutionAPI ChooseParam
{
	//使用的类型
	StrategyType m_useType;
	//使用的计数类型
	StrategyType m_useCountType;
	//是否是观察类型
	bool m_isObserve;
	//解决方案类型，如果是observer则是内部类型，不是则直接使用
	SolutionType m_solutionType;

	ChooseParam()
	{
		m_useType = STRATEGY_INIT;
		m_useCountType = STRATEGY_INIT;
		m_isObserve = false;
		m_solutionType = SOLUTION_INIT;
	}
};