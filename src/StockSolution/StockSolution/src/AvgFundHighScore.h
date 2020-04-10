#pragma once
#include "Solution.h"
#include <memory>

class Strategy;
struct SolutionInfo;
class StockSolutionAPI AvgFundHighScore : public Solution
{
public:
	/** 构造函数
	*/
	AvgFundHighScore();

public:
	/** 选出可以goumai的gupiao
	@param [out] buyStock 选出的gupiao集合，stock,price,rate0-1
	@param [in] date 日期
	@return 是否有选出的gupiao，集合不为0不代表有goumai的gupiao
	*/
	virtual bool buy(std::vector<std::pair<std::string, StockInfo>>& buyStock, const IntDateTime& date);

	/** 询问单只gupiao需不需要maichu
	@param [in] sellStock maichu的gupiao集合，stock,price,rate0-1
	@param [in] date 日期
	@return 返回是否有需要maichu的gupiao，集合不为0不代表有maichu的gupiao
	*/
	virtual bool sell(std::vector<std::pair<std::string, StockInfo>>& sellStock, const IntDateTime& date);

protected:
	/** 根据策略选出可以goumai的gupiao
	@param [out] buyStock 选出的gupiao集合，stock,price,score0-100
	@param [in] date 日期
	@param [in] solutionInfo 解决方案需要的信息
	@return 是否有选出的gupiao
	*/
	bool strategyBuy(std::vector<std::pair<std::string, StockInfo>>& buyStock, const IntDateTime& date);

	/** 获取可mairu的计数
	@param[in] date 日期
	@param[in] solutionInfo 解决方案需要的信息
	@return 返回可mairu的计数
	*/
	int32_t strategyBuyCount(const IntDateTime& date);

	/** 询问单只gupiao需不需要maichu
	@param [in] sellStock maichu的gupiao集合，stock,price,score0-100
	@param [in] date 日期
	@param [in] solutionInfo 解决方案需要的信息
	@return 返回是否有需要maichu的gupiao
	*/
	bool strategySell(std::vector<std::pair<std::string, StockInfo>>& sellStock, const IntDateTime& date);
};