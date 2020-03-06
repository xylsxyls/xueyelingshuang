#pragma once
#include "IndicatorBase.h"
#include "StockIndicatorMacro.h"

/** bollÊý¾Ýzhibiao
*/
struct StockIndicatorAPI StockBoll : public IndicatorBase
{
	BigNumber m_mid;
	BigNumber m_up;
	BigNumber m_down;

	StockBoll()
	{
		m_type = "boll";
	}
};