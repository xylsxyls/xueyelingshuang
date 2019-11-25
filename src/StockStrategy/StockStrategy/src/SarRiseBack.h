#pragma once
#include "Strategy.h"
#include <memory>
#include "StrategyInfo.h"

class StockStrategyAPI SarRiseBack : public Strategy
{
public:
	SarRiseBack();

public:
	/** 单个gupiao是否需要mairu
	@param [in] date 日期
	@param [out] price 价格
	@param [out] score 分数0-100
	@param [in] buyInfo mairu过的信息
	@return 是否需要mairu
	*/
	virtual bool buy(const IntDateTime& date,
		BigNumber& price,
		BigNumber& score,
		const std::shared_ptr<StrategyInfo>& strategyInfo);

	/** 单个gupiao是否需要maichu
	@param [in] date 日期
	@param [out] price 价格
	@param [out] score 分数0-100
	@param [in] buyInfo mairu过的信息
	@return 是否需要maichu
	*/
	virtual bool sell(const IntDateTime& date,
		BigNumber& price,
		BigNumber& score,
		const std::shared_ptr<StrategyInfo>& strategyInfo);

private:
	bool m_isBuy;
	IntDateTime m_buyDate;
	BigNumber m_buyPrice;
};