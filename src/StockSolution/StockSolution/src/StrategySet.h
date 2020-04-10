#pragma once
#include "Solution.h"
#include <memory>

class Strategy;
struct SolutionInfo;
class StockSolutionAPI StrategySet : public Solution
{
public:
	/** 构造函数
	*/
	StrategySet();

public:
	/** 选出可以goumai的gupiao
	@param [out] buyStock 选出的gupiao集合，stock,price,rate0-1
	@param [in] date 日期
	@return 是否有选出的gupiao，集合不为0不代表有goumai的gupiao
	*/
	virtual bool buy(std::vector<std::pair<std::string, StockInfo>>& buyStock, const IntDateTime& date);
};