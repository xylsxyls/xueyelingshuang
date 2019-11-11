#pragma once
#include "Strategy.h"
#include <memory>

class StockMarket;
class StockSarIndicator;
class StockBollIndicator;

class StockStrategyAPI SarRiseBack : public Strategy
{
public:
	SarRiseBack();

public:
	void init(const std::shared_ptr<StockSarIndicator>& stockSarIndicator,
		const std::shared_ptr<StockBollIndicator>& stockBollIndicator);

	/** ���ر�Ҫ����
	*/
	virtual void load();

	/** ����gupiao�Ƿ���Ҫmairu
	@param [in] date ����
	@param [out] price �۸�
	@param [out] date �ٷֱ�
	@param [in] buyInfo mairu������Ϣ
	@return �Ƿ���Ҫmairu
	*/
	virtual bool buy(const IntDateTime& date,
		BigNumber& price,
		BigNumber& percent,
		const std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>& buyInfo);

	/** ����gupiao�Ƿ���Ҫmaichu
	@param [in] date ����
	@param [out] price �۸�
	@param [out] date �ٷֱ�
	@param [in] buyInfo mairu������Ϣ
	@return �Ƿ���Ҫmaichu
	*/
	virtual bool sell(const IntDateTime& date,
		BigNumber& price,
		BigNumber& percent,
		const std::vector<std::pair<IntDateTime, std::pair<BigNumber, BigNumber>>>& buyInfo);

private:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::shared_ptr<StockSarIndicator> m_spStockSarIndicator;
	std::shared_ptr<StockBollIndicator> m_spStockBollIndicator;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	bool m_isBuy;
	IntDateTime m_buyDate;
	BigNumber m_buyPrice;
};