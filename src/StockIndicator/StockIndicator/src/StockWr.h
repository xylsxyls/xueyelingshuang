#pragma once
#include "IndicatorBase.h"
#include "StockIndicatorMacro.h"

class StockIndicatorAPI StockWr : public IndicatorBase
{
	friend StockWrIndicator;
public:
	/** 获取wr10
	@return 返回wr10
	*/
	BigNumber wr10();

	/** 获取wr20
	@return 返回wr20
	*/
	BigNumber wr20();

private:
	BigNumber m_wr10;
	BigNumber m_wr20;
};