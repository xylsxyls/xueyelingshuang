#pragma once
#include "StockStrategyMacro.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndex/StockIndexAPI.h"
#include "StockHandle.h"
#include <map>
#include "StockFund/StockFundAPI.h"

class StockStrategyAPI Strategy
{
	friend class StockStrategy;
public:
	/** 构造函数
	*/
	Strategy();

	/** 析构函数
	*/
	virtual ~Strategy();

public:
	/** 策略
	@param [in] date 日期
	@param [in] validStock 有效的股票集合，股票代码，股票行情，股票指标
	@return 返回买卖的股票，买为true，股票代码，冻结或解冻，都是整数
	*/
	virtual std::vector<StockHandle> strategy(const IntDateTime& date, const std::map<std::string, std::pair<std::shared_ptr<StockMarket>, std::shared_ptr<StockIndex>>>& validStock) = 0;

protected:
	/** 初始化，外部不需要调用
	@param [in] stockFund 资金总仓
	*/
	void init(StockFund* stockFund);

protected:
	//资金总仓
	StockFund* m_stockFund;
};