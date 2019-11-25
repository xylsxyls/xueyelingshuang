#pragma once
#include "Strategy.h"
#include <memory>
#include "StrategyInfo.h"

class StockStrategyAPI SarRiseBack : public Strategy
{
public:
	SarRiseBack();

public:
	/** ����gupiao�Ƿ���Ҫmairu
	@param [in] date ����
	@param [out] price �۸�
	@param [out] score ����0-100
	@param [in] buyInfo mairu������Ϣ
	@return �Ƿ���Ҫmairu
	*/
	virtual bool buy(const IntDateTime& date,
		BigNumber& price,
		BigNumber& score,
		const std::shared_ptr<StrategyInfo>& strategyInfo);

	/** ����gupiao�Ƿ���Ҫmaichu
	@param [in] date ����
	@param [out] price �۸�
	@param [out] score ����0-100
	@param [in] buyInfo mairu������Ϣ
	@return �Ƿ���Ҫmaichu
	*/
	virtual bool sell(const IntDateTime& date,
		BigNumber& price,
		BigNumber& score,
		const std::shared_ptr<StrategyInfo>& strategyInfo);

private:
	bool m_isBuy;
	IntDateTime m_buyDate;
	BigNumber m_buyPrice;
};