#pragma once
#include "Strategy.h"
#include <memory>
#include "StrategyInfo.h"

class StockStrategyAPI SarRiseBackCount : public Strategy
{
public:
	/** ���캯��
	*/
	SarRiseBackCount();

public:
	/** ����gupiao�Ƿ���Ҫmairu
	@param [in] date ����
	@param [out] price �۸�
	@param [out] percent �ٷֱ�0-100
	@param [out] score ����
	@param [in] strategyInfo ������Ҫ����Ϣ
	@return �Ƿ���Ҫmairu
	*/
	virtual bool buy(const IntDateTime& date,
		BigNumber& price,
		BigNumber& percent,
		BigNumber& score,
		const std::shared_ptr<StrategyInfo>& strategyInfo);

	/** ��ȡ��Ҫ��zhibiao
	@return ������Ҫ��zhibiao
	*/
	virtual std::set<std::string> needIndicator();
};