#pragma once
#include "Strategy.h"
#include <memory>
#include "StrategyInfo.h"

class StockStrategyAPI T0 : public Strategy
{
public:
	/** ���캯��
	*/
	T0();

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

	/** �۲����͵���gupiao�Ƿ���Ҫmaichu
	@param [in] date ����
	@param [in] stockInfo ѡ����Ϣ
	@return �Ƿ���Ҫmaichu
	*/
	virtual bool observeSell(const IntDateTime& date, StockInfo& stockInfo);
};