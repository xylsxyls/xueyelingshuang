#pragma once
#include "Strategy.h"
#include <memory>
#include "StrategyInfo.h"

class StockStrategyAPI SarRiseBack : public Strategy
{
public:
	/** ���캯��
	*/
	SarRiseBack();

public:
	/** ����gupiao�Ƿ���Ҫmairu
	@param [in] date ����
	@param [out] price �۸�
	@param [out] percent �ٷֱ�0-100
	@param [out] score ����
	@param [in] buyInfo mairu������Ϣ
	@return �Ƿ���Ҫmairu
	*/
	virtual bool buy(const IntDateTime& date,
		BigNumber& price,
		BigNumber& percent,
		BigNumber& score,
		const std::shared_ptr<StrategyInfo>& strategyInfo);

	/** ����gupiao�Ƿ���Ҫmaichu
	@param [in] date ����
	@param [out] price �۸�
	@param [out] percent �ٷֱ�0-100
	@param [out] score ����
	@param [in] buyInfo mairu������Ϣ
	@return �Ƿ���Ҫmaichu
	*/
	virtual bool sell(const IntDateTime& date,
		BigNumber& price,
		BigNumber& percent,
		BigNumber& score,
		const std::shared_ptr<StrategyInfo>& strategyInfo);

	/** ��ȡgupiaochi�������ú�����������buy֮��ʹ��
	@return ����gupiaochi����
	*/
	virtual int32_t buyPollSize();

private:
	IntDateTime m_buyDate;
	BigNumber m_buyPrice;
};