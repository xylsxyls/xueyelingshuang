#pragma once
#include "StockSolutionMacro.h"
#include <map>
#include <memory>

struct StrategyInfo;
struct StockSolutionAPI SolutionAllInfo
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<std::string, std::shared_ptr<StrategyInfo>> m_strategyAllInfo;
	std::vector<std::string>* m_filterStock;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	SolutionAllInfo()
	{
		m_filterStock = nullptr;
	}

	virtual ~SolutionAllInfo(){}
};