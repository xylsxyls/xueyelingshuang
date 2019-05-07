#pragma once
#include "StockStrategyMacro.h"
#include "StockMarket/StockMarketAPI.h"
#include "StockIndex/StockIndexAPI.h"
#include "StockHandle.h"
#include <map>
#include "StockFund/StockFundAPI.h"

class StockStrategyAPI Strategy
{
	friend class StockStrategy;
public:
	/** ���캯��
	*/
	Strategy();

	/** ��������
	*/
	virtual ~Strategy();

public:
	/** ����
	@param [in] date ����
	@param [in] validStock ��Ч�Ĺ�Ʊ���ϣ���Ʊ���룬��Ʊ���飬��Ʊָ��
	@return ���������Ĺ�Ʊ����Ϊtrue����Ʊ���룬�����ⶳ����������
	*/
	virtual std::vector<StockHandle> strategy(const IntDateTime& date, const std::map<std::string, std::pair<std::shared_ptr<StockMarket>, std::shared_ptr<StockIndex>>>& validStock) = 0;

protected:
	/** ��ʼ�����ⲿ����Ҫ����
	@param [in] stockFund �ʽ��ܲ�
	*/
	void init(StockFund* stockFund);

protected:
	//�ʽ��ܲ�
	StockFund* m_stockFund;
};