#pragma once
#include "Strategy.h"
#include <memory>

class StockMarket;
class StockSarIndicator;
class StockBollIndicator;

class SarRiseBack : public Strategy
{
public:
	SarRiseBack();

public:
	void init(const std::shared_ptr<StockMarket>& spMarket,
		const std::shared_ptr<StockSarIndicator>& stockSarIndicator,
		const std::shared_ptr<StockBollIndicator>& stockBollIndicator);

	/** ���ر�Ҫ����
	*/
	virtual void load();

	/** ����gupiao�Ƿ���Ҫmairu
	@param [in] date ����
	@param [out] price �۸�
	@param [out] date �ٷֱ�
	@return �Ƿ���Ҫmairu
	*/
	virtual bool buy(const IntDateTime& date, BigNumber& price, BigNumber& percent);

	/** ����gupiao�Ƿ���Ҫmaichu
	@param [in] date ����
	@param [out] price �۸�
	@param [out] date �ٷֱ�
	@return �Ƿ���Ҫmaichu
	*/
	virtual bool sell(const IntDateTime& date, BigNumber& price, BigNumber& percent);

private:
	std::shared_ptr<StockMarket> m_spMarket;
	std::shared_ptr<StockSarIndicator> m_spStockSarIndicator;
	std::shared_ptr<StockBollIndicator> m_spStockBollIndicator;
	bool m_isBuy;
	IntDateTime m_buyDate;
	BigNumber m_buyPrice;
};