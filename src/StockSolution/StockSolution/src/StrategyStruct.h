#pragma once
#include <memory>
#include "StockSolutionMacro.h"

class Strategy;
struct StrategyInfo;

struct StockSolutionAPI StrategyStruct
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<Strategy> m_strategy;
	std::shared_ptr<Strategy> m_strategyCount;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};

struct StockSolutionAPI StrategyInfoStruct
{
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<StrategyInfo> m_strategyInfo;
	std::shared_ptr<StrategyInfo> m_strategyCountInfo;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};