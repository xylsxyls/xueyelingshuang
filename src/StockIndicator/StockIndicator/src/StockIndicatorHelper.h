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
	@return ����sar����zhibiao��zhibiao��ֵ��״̬��12340�ֱ����죬�̣���ת�̣���ת�죬��Чֵ����ö��ֵ��Ӧ
	*/
	static std::map<IntDateTime, std::pair<BigNumber, int32_t>> sar(StockMarket& stockMarket,
		int32_t cycle,
		const BigNumber& afBegin = "0.02",
		const BigNumber& afSpeed = "0.02",
		const BigNumber& afMax = "0.2");

	/** bollzhibiao
	@param [in] date ��������
	@param [in] stockMarket hangqing
	@param [in] days ����
	@param [in] expand ����ϵ��
	@return ����bollzhibiao����������ڷ���-1��������
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