#pragma once
#include "Strategy.h"
#include <memory>
#include "StrategyInfo.h"

class StockStrategyAPI SarRiseBack : public Strategy
{
public:
	/** 构造函数
	*/
	SarRiseBack();

public:
	/** 单个gupiao是否需要mairu
	@param [in] date 日期
	@param [out] price 价格
	@param [out] percent 百分比0-100
	@param [out] score 分数
	@param [in] buyInfo mairu过的信息
	@return 是否需要mairu
	*/
	virtual bool buy(const IntDateTime& date,
		BigNumber& price,
		BigNumber& percent,
		BigNumber& score,
		const std::shared_ptr<StrategyInfo>& strategyInfo);

	/** 单个gupiao是否需要maichu
	@param [in] date 日期
	@param [out] price 价格
	@param [out] percent 百分比0-100
	@param [out] score 分数
	@param [in] buyInfo mairu过的信息
	@return 是否需要maichu
	*/
	virtual bool sell(const IntDateTime& date,
		BigNumber& price,
		BigNumber& percent,
		BigNumber& score,
		const std::shared_ptr<StrategyInfo>& strategyInfo);

	/** 获取gupiaochi个数，该函数需在运行buy之后使用
	@return 返回gupiaochi个数
	*/
	virtual int32_t buyPollSize();

private:
	IntDateTime m_buyDate;
	BigNumber m_buyPrice;
};