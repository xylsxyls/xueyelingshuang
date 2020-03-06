#pragma once
#include "IndicatorBase.h"
#include "StockIndicatorMacro.h"

/** boll����zhibiao
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