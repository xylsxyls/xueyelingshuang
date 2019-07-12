#pragma once
#include "IndicatorBase.h"
#include "StockIndicatorMacro.h"

class StockIndicatorAPI StockWr : public IndicatorBase
{
	friend StockWrIndicator;
public:
	/** ��ȡwr10
	@return ����wr10
	*/
	BigNumber wr10();

	/** ��ȡwr20
	@return ����wr20
	*/
	BigNumber wr20();

private:
	BigNumber m_wr10;
	BigNumber m_wr20;
};