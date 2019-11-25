#pragma once
#include "StockStrategyMacro.h"
#include <memory>
#include "IntDateTime/IntDateTimeAPI.h"
#include <set>

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
	std::vector<std::string> strategyAllStock(StrategyEnum strategyEnum, const IntDateTime& beginTime, const IntDateTime& endTime);
	void strategyStock(const IntDateTime& date, std::vector<std::string>& filterStock);

	std::shared_ptr<Strategy> strategy(StrategyEnum strategyEnum);
	std::set<std::string> strategyNeedLoad(StrategyEnum strategyEnum);
};