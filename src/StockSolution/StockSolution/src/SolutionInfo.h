#pragma once
#include "StockSolutionMacro.h"
#include <map>
#include <memory>

struct StrategyInfo;
class StockFund;
struct StockSolutionAPI SolutionInfo
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	//stock
	std::map<std::string, std::vector<std::pair<StrategyType, std::pair<std::shared_ptr<StrategyInfo>, std::shared_ptr<StrategyInfo>>>>> m_strategyAllInfo;
	StockFund* m_fund;
	std::vector<std::string>* m_filterStock;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

	SolutionInfo()
	{
		m_fund = nullptr;
		m_filterStock = nullptr;
	}

	virtual ~SolutionInfo()
	{

	}
};