#pragma once
#include "BigNumber/BigNumberAPI.h"
#include <vector>
#include "IntDateTime/IntDateTimeAPI.h"

/** 策略
*/
class Strategy
{
public:
	/** 加载必要数据
	*/
	virtual void load() = 0;

	/** 单个gupiao是否需要mairu
	@param [in] date 日期
	@param [out] price 价格
	@param [out] date 百分比
	@return 是否需要mairu
	*/
	virtual bool buy(const IntDateTime& date, BigNumber& price, BigNumber& percent) = 0;

	/** 单个gupiao是否需要maichu
	@param [in] date 日期
	@param [out] price 价格
	@param [out] date 百分比
	@return 是否需要maichu
	*/
	virtual bool sell(const IntDateTime& date, BigNumber& price, BigNumber& percent) = 0;
};