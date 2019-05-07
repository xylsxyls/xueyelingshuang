#pragma once
#include "StockStrategy/StockStrategyAPI.h"
#include <memory>

class StockWrRsi : public Strategy
{
public:
	/** 策略
	@param [in] date 日期
	@param [in] validStock 有效的股票集合，股票代码，股票行情，股票指标
	@return 返回买卖的股票，买为true，股票代码，冻结或解冻，都是整数
	*/
	virtual std::vector<StockHandle> strategy(const IntDateTime& date, const std::map<std::string, std::pair<std::shared_ptr<StockMarket>, std::shared_ptr<StockIndex>>>& validStock);

	/** 购买逻辑
	@param [in] date 日期
	@param [in] validStock 有效的股票集合，股票代码，股票行情，股票指标
	@return 返回可购买的股票集合，股票代码，购买价格
	*/
	std::vector<std::pair<std::string, BigNumber>> availBuyStock(const IntDateTime& date, const std::map<std::string, std::pair<std::shared_ptr<StockMarket>, std::shared_ptr<StockIndex>>>& validStock);

	/** 补买逻辑
	@param [in] date 日期
	@param [in] stockMarket 股票行情
	@param [in] stockIndex 股票指标
	@return 返回是否可补买
	*/
	bool addBuy(const IntDateTime& date, const std::shared_ptr<StockMarket>& stockMarket, const std::shared_ptr<StockIndex>& stockIndex);

	/** 卖出逻辑
	@param [in] date 日期
	@param [in] stockMarket 股票行情
	@param [in] stockIndex 股票指标
	@return 返回是否需要卖出
	*/
	bool needSell(const IntDateTime& date, const std::shared_ptr<StockMarket>& stockMarket, const std::shared_ptr<StockIndex>& stockIndex);
};