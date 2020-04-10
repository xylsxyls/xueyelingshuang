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
	@param [in] stockInfo ѡ����Ϣ
	@return �Ƿ���Ҫmairu
	*/
	virtual bool buy(const IntDateTime& date, StockInfo& stockInfo);
};