#pragma once
#include "StockStrategyMacro.h"
#include <memory>
#include "IntDateTime/IntDateTimeAPI.h"
#include <set>
#include "Strategy.h"

class Strategy;
class StockStrategyAPI StockStrategy
{
protected:
	StockStrategy();

public:
	static StockStrategy& instance();

public:
	std::vector<std::string> strategyAllStock(StrategyType strategyEnum, const IntDateTime& beginTime, const IntDateTime& endTime);
	void strategyStock(const IntDateTime& date, std::vector<std::string>& filterStock);

	std::shared_ptr<Strategy> strategy(StrategyType strategyEnum);
	std::set<std::string> strategyNeedLoad(StrategyType strategyEnum);
};