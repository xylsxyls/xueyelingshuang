#pragma once
#include "StockSolutionMacro.h"

struct StrategyInfo;
struct StockSolutionAPI SolutionInfo
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<StrategyInfo> m_strategyInfo;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	SolutionInfo()
	{
		m_strategyInfo = nullptr;
	}

	virtual ~SolutionInfo()
	{

	}
};