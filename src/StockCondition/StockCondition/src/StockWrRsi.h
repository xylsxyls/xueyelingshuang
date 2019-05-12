#pragma once
#include "StockStrategy/StockStrategyAPI.h"
#include <memory>

class StockWrRsi : public Strategy
{
public:
	/** ����
	@param [in] date ����
	@param [in] validStock ��Ч�Ĺ�Ʊ���ϣ���Ʊ���룬��Ʊ���飬��Ʊָ��
	@param [out] ���������Ĺ�Ʊ����Ϊtrue����Ʊ���룬�����ⶳ����������
	*/
	virtual void strategy(const IntDateTime& date, const std::map<std::string, std::pair<std::shared_ptr<StockMarket>, std::shared_ptr<StockIndex>>>& validStock, std::vector<StockHandle>& vecStockHandle);

	/** �����߼�
	@param [in] date ����
	@param [in] validStock ��Ч�Ĺ�Ʊ���ϣ���Ʊ���룬��Ʊ���飬��Ʊָ��
	@param [out] vecAvailBuyStock �����ɹ���Ĺ�Ʊ���ϣ���Ʊ���룬����۸�
	*/
	void availBuyStock(const IntDateTime& date,
		const std::map<std::string, std::pair<std::shared_ptr<StockMarket>, std::shared_ptr<StockIndex>>>& validStock,
		std::vector<std::pair<std::string, BigNumber>>& vecAvailBuyStock);

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

private:
	StockHandle m_stockHandle;
	std::vector<std::pair<std::string, BigNumber>> m_vecChooseStock;
	std::map <BigNumber, std::vector<std::pair<std::string, BigNumber>>> m_mapChooseStock;
	std::pair<std::string, BigNumber> m_chooseStock;
};