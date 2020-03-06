#pragma once
#include "IndicatorBase.h"
#include "StockIndicatorMacro.h"

struct StockIndicatorAPI StockWr : public IndicatorBase
{
	BigNumber m_wr10;
	BigNumber m_wr20;

	StockWr()
	{
		m_type = "wr";
	}
};