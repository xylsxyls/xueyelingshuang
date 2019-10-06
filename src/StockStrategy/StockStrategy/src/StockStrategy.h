#pragma once
#include "StockStrategyMacro.h"
#include <memory>

class Strategy;
class StockMarket;
class StockWrIndicator;
class StockRsiIndicator;
class StockSarIndicator;
class StockBollIndicator;

enum StrategyEnum
{
	STRATEGY_INIT,

	SAR_RISE_BACK
};

class StockStrategyAPI StockStrategy
{
protected:
	StockStrategy();

public:
	static StockStrategy& instance();

public:
	std::vector<std::string> strategyAllStock();

	std::shared_ptr<Strategy> strategy(const std::string& stock,
		const std::shared_ptr<StockMarket>& stockMarket,
		const std::shared_ptr<StockWrIndicator>& stockWrIndicator,
		const std::shared_ptr<StockRsiIndicator>& stockRsiIndicator,
		const std::shared_ptr<StockSarIndicator>& stockSarIndicator,
		const std::shared_ptr<StockBollIndicator>& stockBollIndicator,
		StrategyEnum strategyEnum);
};