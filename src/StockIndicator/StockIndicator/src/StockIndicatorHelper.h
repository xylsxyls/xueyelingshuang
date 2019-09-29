#pragma once
#include "IntDateTime/IntDateTimeAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <map>
#include <memory>

class StockMarket;
/** zhibiao������
*/
class StockIndicatorHelper
{
public:
	/** wrzhibiao
	@param [in] days ����
	@param [in] date ��������
	@param [in] stockMarket hangqing
	@return ����rsizhibiao����������ڷ���-1
	*/
	static BigNumber wr(int32_t days, const IntDateTime& date, StockMarket& stockMarket);

	/** rsizhibiao
	@param [in] days ����
	@param [in] date ��������
	@param [in] stockMarket hangqing
	@return ����rsizhibiao����������ڷ���-1
	*/
	static BigNumber rsi(int32_t days, const IntDateTime& date, StockMarket& stockMarket);

	/** rsitonghuashunzhibiao
	@param [in] days ����
	@param [in] stockMarket hangqing
	@return ����rsi����zhibiao
	*/
	static std::map<IntDateTime, BigNumber> rsiTongHuaShun(int32_t days, StockMarket& stockMarket);

	/** sarzhibiao
	@param [in] stockMarket hangqing
	@return ����sar����zhibiao
	*/
	static std::map<IntDateTime, BigNumber> sar(StockMarket& stockMarket);

protected:
	static BigNumber SMA(const BigNumber& X, const BigNumber& N, const BigNumber& M);
	static BigNumber MAX(const BigNumber& X, const BigNumber& Y);
	static BigNumber ABS(const BigNumber& X);
};