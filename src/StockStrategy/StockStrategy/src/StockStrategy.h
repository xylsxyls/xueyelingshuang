#pragma once
#include "StockStrategyMacro.h"
#include "IntDateTime/IntDateTimeAPI.h"

class Strategy;
class StockFund;
class StockStrategyAPI StockStrategy
{
public:
	/** ��ʼ��
	@param [in] strategy ����
	@param [in] stockFund �ʽ��ܲ�
	*/
	void init(Strategy* strategy, StockFund* stockFund);

	/** �ز�
	@param [in] vecStock ��Ʊ����
	@param [in] beginTime ��ʼʱ��
	@param [in] endTime ����ʱ��
	*/
	void run(const std::vector<std::string>& vecStock, const IntDateTime& beginTime, const IntDateTime& endTime);

protected:
	/** ����ڲ�ָ���Ƿ���Ч
	@return �����ڲ�ָ���Ƿ���Ч
	*/
	bool check();

private:
	Strategy* m_strategy;
	StockFund* m_stockFund;
};