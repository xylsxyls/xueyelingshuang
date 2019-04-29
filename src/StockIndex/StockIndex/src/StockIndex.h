#pragma once
#include "StockIndexMacro.h"
#include "StockMarket/StockMarketAPI.h"

class StockIndexAPI StockIndex
{
public:
	/** rsiָ��
	@param [in] days ����
	@param [in] date ��������
	@param [in] stockMarket ����
	@return ����rsiָ�꣬��������ڷ���-1
	*/
	static BigNumber rsi(int32_t days, const IntDateTime& date, const StockMarket& stockMarket);

	/** wrָ��
	@param [in] days ����
	@param [in] date ��������
	@param [in] stockMarket ����
	@return ����rsiָ�꣬��������ڷ���-1
	*/
	static BigNumber wr(int32_t days, const IntDateTime& date, const StockMarket& stockMarket);

private:
	static BigNumber SMA(const BigNumber& X, const BigNumber& N, const BigNumber& M);
	static BigNumber MAX(const BigNumber& X, const BigNumber& Y);
	static BigNumber ABS(const BigNumber& X);
};