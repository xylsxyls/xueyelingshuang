#pragma once
#include "Strategy.h"
#include <memory>
#include "StrategyInfo.h"

class StockStrategyAPI SarRiseBackThirtyLine : public Strategy
{
public:
	/** ���캯��
	*/
	SarRiseBackThirtyLine();

public:
	/** ����gupiao�Ƿ���Ҫmairu
	@param [in] date ����
	@param [in] stockInfo ѡ����Ϣ
	@return �Ƿ���Ҫmairu
	*/
	virtual bool buy(const IntDateTime& date, StockInfo& stockInfo);

	/** ����gupiao�Ƿ���Ҫmaichu
	@param [in] date ����
	@param [in] stockInfo ѡ����Ϣ
	@return �Ƿ���Ҫmaichu
	*/
	virtual bool sell(const IntDateTime& date, StockInfo& stockInfo);
};