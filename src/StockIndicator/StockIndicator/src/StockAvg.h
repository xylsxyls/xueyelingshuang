#pragma once
#include "IndicatorBase.h"
#include "StockIndicatorMacro.h"

struct StockIndicatorAPI StockAvg : public IndicatorBase
{
	BigNumber m_avg09_30;
	BigNumber m_avg10_00;
	BigNumber m_avg10_30;
	BigNumber m_avg11_00;
	BigNumber m_avg11_30;
	BigNumber m_avg13_30;
	BigNumber m_avg14_00;
	BigNumber m_avg14_30;
	BigNumber m_avg15_00;
	BigNumber m_avgHigh;
	BigNumber m_avgLow;

	StockAvg()
	{
		m_type = "avg";
	}
};