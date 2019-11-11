#pragma once
#include "StockStrategyMacro.h"
#include <memory>
#include "IntDateTime/IntDateTimeAPI.h"

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
	std::vector<std::string> strategyStock(StrategyEnum strategyEnum, const IntDateTime& date);

	std::shared_ptr<Strategy> strategy(const std::string& stock,
		const std::shared_ptr<StockMarket>& spMarket,
		StrategyEnum strategyEnum);
};