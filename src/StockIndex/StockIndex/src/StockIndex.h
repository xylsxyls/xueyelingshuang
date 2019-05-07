#pragma once
#include "StockIndexMacro.h"
#include "BigNumber/BigNumberAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include <memory>
#include <map>

class StockMarket;
class StockIndexAPI StockIndex
{
public:
	/** rsi指标
	@param [in] days 周期
	@param [in] date 计算日期
	@param [in] stockMarket 行情
	@return 返回rsi指标，如果不存在返回-1
	*/
	static BigNumber rsi(int32_t days, const IntDateTime& date, const std::shared_ptr<StockMarket>& stockMarket);

	/** wr指标
	@param [in] days 周期
	@param [in] date 计算日期
	@param [in] stockMarket 行情
	@return 返回rsi指标，如果不存在返回-1
	*/
	static BigNumber wr(int32_t days, const IntDateTime& date, const std::shared_ptr<StockMarket>& stockMarket);

	/** 加载指标
	@param [in] mysql 数据库
	*/
	void load(const std::map<IntDateTime, std::vector<BigNumber>>& data);

	/** 获取RSI6
	@param [in] date 日期
	@return 返回RSI6
	*/
	BigNumber rsi6(const IntDateTime& date) const;

	/** 获取RSI12
	@param [in] date 日期
	@return 返回RSI12
	*/
	BigNumber rsi12(const IntDateTime& date) const;

	/** 获取RSI24
	@param [in] date 日期
	@return 返回RSI24
	*/
	BigNumber rsi24(const IntDateTime& date) const;

	/** 获取wr10
	@param [in] date 日期
	@return 返回wr10
	*/
	BigNumber wr10(const IntDateTime& date) const;

	/** 获取wr20
	@param [in] date 日期
	@return 返回wr20
	*/
	BigNumber wr20(const IntDateTime& date) const;

private:
	static BigNumber SMA(const BigNumber& X, const BigNumber& N, const BigNumber& M);
	static BigNumber MAX(const BigNumber& X, const BigNumber& Y);
	static BigNumber ABS(const BigNumber& X);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::map<IntDateTime, std::vector<BigNumber>> m_stockIndex;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};