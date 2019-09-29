#pragma once
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <map>
#include <memory>

class StockMarket;
/** zhibiao计算类
*/
class StockIndicatorHelper
{
public:
	/** wrzhibiao
	@param [in] days 周期
	@param [in] date 计算日期
	@param [in] stockMarket hangqing
	@return 返回rsizhibiao，如果不存在返回-1
	*/
	static BigNumber wr(int32_t days, const IntDateTime& date, StockMarket& stockMarket);

	/** rsizhibiao
	@param [in] days 周期
	@param [in] date 计算日期
	@param [in] stockMarket hangqing
	@return 返回rsizhibiao，如果不存在返回-1
	*/
	static BigNumber rsi(int32_t days, const IntDateTime& date, StockMarket& stockMarket);

	/** rsitonghuashunzhibiao
	@param [in] days 周期
	@param [in] stockMarket hangqing
	@return 返回rsi所有zhibiao
	*/
	static std::map<IntDateTime, BigNumber> rsiTongHuaShun(int32_t days, StockMarket& stockMarket);

	/** sarzhibiao
	@param [in] stockMarket hangqing
	@return 返回sar所有zhibiao
	*/
	static std::map<IntDateTime, BigNumber> sar(StockMarket& stockMarket);

protected:
	static BigNumber SMA(const BigNumber& X, const BigNumber& N, const BigNumber& M);
	static BigNumber MAX(const BigNumber& X, const BigNumber& Y);
	static BigNumber ABS(const BigNumber& X);
};