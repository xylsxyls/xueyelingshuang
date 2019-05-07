#pragma once
#include "StockStrategy/StockStrategyAPI.h"
#include <memory>

class StockWrRsi : public Strategy
{
public:
	/** ����
	@param [in] date ����
	@param [in] validStock ��Ч�Ĺ�Ʊ���ϣ���Ʊ���룬��Ʊ���飬��Ʊָ��
	@return ���������Ĺ�Ʊ����Ϊtrue����Ʊ���룬�����ⶳ����������
	*/
	virtual std::vector<StockHandle> strategy(const IntDateTime& date, const std::map<std::string, std::pair<std::shared_ptr<StockMarket>, std::shared_ptr<StockIndex>>>& validStock);

	/** �����߼�
	@param [in] date ����
	@param [in] validStock ��Ч�Ĺ�Ʊ���ϣ���Ʊ���룬��Ʊ���飬��Ʊָ��
	@return ���ؿɹ���Ĺ�Ʊ���ϣ���Ʊ���룬����۸�
	*/
	std::vector<std::pair<std::string, BigNumber>> availBuyStock(const IntDateTime& date, const std::map<std::string, std::pair<std::shared_ptr<StockMarket>, std::shared_ptr<StockIndex>>>& validStock);

	/** �����߼�
	@param [in] date ����
	@param [in] stockMarket ��Ʊ����
	@param [in] stockIndex ��Ʊָ��
	@return �����Ƿ�ɲ���
	*/
	bool addBuy(const IntDateTime& date, const std::shared_ptr<StockMarket>& stockMarket, const std::shared_ptr<StockIndex>& stockIndex);

	/** �����߼�
	@param [in] date ����
	@param [in] stockMarket ��Ʊ����
	@param [in] stockIndex ��Ʊָ��
	@return �����Ƿ���Ҫ����
	*/
	bool needSell(const IntDateTime& date, const std::shared_ptr<StockMarket>& stockMarket, const std::shared_ptr<StockIndex>& stockIndex);
};