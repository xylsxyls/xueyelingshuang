#pragma once
#include "StockSolutionMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include <memory>
#include "StockMarket/StockMarketAPI.h"
#include "Solution.h"

class StockSolutionAPI StockSolution
{
protected:
	StockSolution();

public:
	static StockSolution& instance();

public:
	std::shared_ptr<Solution> solution(SolutionType strategyEnum);
};