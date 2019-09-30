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
	@return 返回sar所有zhibiao，zhibiao数值和状态，12340分别代表红，绿，红转绿，绿转红，无效值，与枚举值对应
	*/
	static std::map<IntDateTime, std::pair<BigNumber, int32_t>> sar(StockMarket& stockMarket,
		int32_t cycle,
		const BigNumber& afBegin = "0.02",
		const BigNumber& afSpeed = "0.02",
		const BigNumber& afMax = "0.2");

	/** bollzhibiao
	@param [in] date 计算日期
	@param [in] stockMarket hangqing
	@param [in] days 周期
	@param [in] expand 扩张系数
	@return 返回bollzhibiao，如果不存在返回-1，中上下
	*/
	static std::pair < BigNumber, std::pair<BigNumber, BigNumber>> boll(const IntDateTime& date,
		StockMarket& stockMarket,
		int32_t days = 20,
		const BigNumber& expand = 2);

protected:
	static BigNumber SMA(const BigNumber& X, const BigNumber& N, const BigNumber& M);
	static BigNumber MAX(const BigNumber& X, const BigNumber& Y);
	static BigNumber ABS(const BigNumber& X);
};