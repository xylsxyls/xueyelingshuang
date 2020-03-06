#pragma once
#include "IndicatorBase.h"
#include "StockIndicatorMacro.h"

struct StockIndicatorAPI StockRsi : public IndicatorBase
{
	BigNumber m_rsi6;
	BigNumber m_rsi12;
	BigNumber m_rsi24;

	StockRsi()
	{
		m_type = "rsi";
	}
};