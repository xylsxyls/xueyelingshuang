#pragma once
#include "IndicatorBase.h"
#include "StockIndicatorMacro.h"

class StockIndicatorAPI StockRsi : public IndicatorBase
{
	friend class StockRsiIndicator;
public:
	/** ��ȡrsi6
	@return ����rsi6
	*/
	BigNumber rsi6();

	/** ��ȡrsi12
	@return ����rsi12
	*/
	BigNumber rsi12();

	/** ��ȡrsi24
	@return ����rsi24
	*/
	BigNumber rsi24();

private:
	BigNumber m_rsi6;
	BigNumber m_rsi12;
	BigNumber m_rsi24;
};