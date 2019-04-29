#pragma once
#include "StockIndexMacro.h"
#include "StockMarket/StockMarketAPI.h"

class StockIndexAPI StockIndex
{
public:
	/** rsi指标
	@param [in] days 周期
	@param [in] date 计算日期
	@param [in] stockMarket 行情
	@return 返回rsi指标，如果不存在返回-1
	*/
	static BigNumber rsi(int32_t days, const IntDateTime& date, const StockMarket& stockMarket);

	/** wr指标
	@param [in] days 周期
	@param [in] date 计算日期
	@param [in] stockMarket 行情
	@return 返回rsi指标，如果不存在返回-1
	*/
	static BigNumber wr(int32_t days, const IntDateTime& date, const StockMarket& stockMarket);

private:
	static BigNumber SMA(const BigNumber& X, const BigNumber& N, const BigNumber& M);
	static BigNumber MAX(const BigNumber& X, const BigNumber& Y);
	static BigNumber ABS(const BigNumber& X);
};