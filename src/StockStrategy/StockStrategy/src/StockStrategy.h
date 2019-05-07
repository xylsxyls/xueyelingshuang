#pragma once
#include "StockStrategyMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"

class Strategy;
class StockFund;
class StockStrategyAPI StockStrategy
{
public:
	/** 初始化
	@param [in] strategy 策略
	@param [in] stockFund 资金总仓
	*/
	void init(Strategy* strategy, StockFund* stockFund);

	/** 回测
	@param [in] vecStock 股票集合
	@param [in] beginTime 开始时间
	@param [in] endTime 结束时间
	*/
	void run(const std::vector<std::string>& vecStock, const IntDateTime& beginTime, const IntDateTime& endTime);

protected:
	/** 检测内部指针是否有效
	@return 返回内部指针是否有效
	*/
	bool check();

private:
	Strategy* m_strategy;
	StockFund* m_stockFund;
};