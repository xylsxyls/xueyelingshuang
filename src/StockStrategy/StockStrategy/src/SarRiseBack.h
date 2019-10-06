#pragma once
#include "Strategy.h"
#include <memory>

class StockMarket;
class StockSarIndicator;
class StockBollIndicator;

class SarRiseBack : public Strategy
{
public:
	SarRiseBack();

public:
	void init(const std::shared_ptr<StockMarket>& spMarket,
		const std::shared_ptr<StockSarIndicator>& stockSarIndicator,
		const std::shared_ptr<StockBollIndicator>& stockBollIndicator);

	/** 加载必要数据
	*/
	virtual void load();

	/** 单个gupiao是否需要mairu
	@param [in] date 日期
	@param [out] price 价格
	@param [out] date 百分比
	@return 是否需要mairu
	*/
	virtual bool buy(const IntDateTime& date, BigNumber& price, BigNumber& percent);

	/** 单个gupiao是否需要maichu
	@param [in] date 日期
	@param [out] price 价格
	@param [out] date 百分比
	@return 是否需要maichu
	*/
	virtual bool sell(const IntDateTime& date, BigNumber& price, BigNumber& percent);

private:
	std::shared_ptr<StockMarket> m_spMarket;
	std::shared_ptr<StockSarIndicator> m_spStockSarIndicator;
	std::shared_ptr<StockBollIndicator> m_spStockBollIndicator;
	bool m_isBuy;
	IntDateTime m_buyDate;
	BigNumber m_buyPrice;
};