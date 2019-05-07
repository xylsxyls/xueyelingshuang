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
	/** rsiָ��
	@param [in] days ����
	@param [in] date ��������
	@param [in] stockMarket ����
	@return ����rsiָ�꣬��������ڷ���-1
	*/
	static BigNumber rsi(int32_t days, const IntDateTime& date, const std::shared_ptr<StockMarket>& stockMarket);

	/** wrָ��
	@param [in] days ����
	@param [in] date ��������
	@param [in] stockMarket ����
	@return ����rsiָ�꣬��������ڷ���-1
	*/
	static BigNumber wr(int32_t days, const IntDateTime& date, const std::shared_ptr<StockMarket>& stockMarket);

	/** ����ָ��
	@param [in] mysql ���ݿ�
	*/
	void load(const std::map<IntDateTime, std::vector<BigNumber>>& data);

	/** ��ȡRSI6
	@param [in] date ����
	@return ����RSI6
	*/
	BigNumber rsi6(const IntDateTime& date) const;

	/** ��ȡRSI12
	@param [in] date ����
	@return ����RSI12
	*/
	BigNumber rsi12(const IntDateTime& date) const;

	/** ��ȡRSI24
	@param [in] date ����
	@return ����RSI24
	*/
	BigNumber rsi24(const IntDateTime& date) const;

	/** ��ȡwr10
	@param [in] date ����
	@return ����wr10
	*/
	BigNumber wr10(const IntDateTime& date) const;

	/** ��ȡwr20
	@param [in] date ����
	@return ����wr20
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