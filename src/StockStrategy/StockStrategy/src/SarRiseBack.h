#pragma once
#include "Strategy.h"
#include <memory>

class StockMarket;
class StockSarIndicator;
class StockBollIndicator;

class StockStrategyAPI SarRiseBack : public Strategy
{
public:
	SarRiseBack();

public:
	void init(const std::shared_ptr<StockSarIndicator>& stockSarIndicator,
		const std::shared_ptr<StockBollIndicator>& stockBollIndicator);

	/** 加载必要数据
	*/
	virtual void load();

	/** 单个gupiao是否需要mairu
	@param [in] date 日期
	@param [out] price 价格
	@param [out] date 百分比
	@param [in] buyInfo mairu过的信息
	@return 是否需要mairu
	*/
	virtual bool buy(const IntDateTime& date,
		BigNumber& price,
		BigNumber& percent,
		const std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>& buyInfo);

	/** 单个gupiao是否需要maichu
	@param [in] date 日期
	@param [out] price 价格
	@param [out] date 百分比
	@param [in] buyInfo mairu过的信息
	@return 是否需要maichu
	*/
	virtual bool sell(const IntDateTime& date,
		BigNumber& price,
		BigNumber& percent,
		const std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>& buyInfo);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<StockSarIndicator> m_spStockSarIndicator;
	std::shared_ptr<StockBollIndicator> m_spStockBollIndicator;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	bool m_isBuy;
	IntDateTime m_buyDate;
	BigNumber m_buyPrice;
};