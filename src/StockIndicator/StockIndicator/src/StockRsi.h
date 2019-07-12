#pragma once
#include "IndicatorBase.h"
#include "StockIndicatorMacro.h"

class StockIndicatorAPI StockRsi : public IndicatorBase
{
	friend class StockRsiIndicator;
public:
	/** 获取rsi6
	@return 返回rsi6
	*/
	BigNumber rsi6();

	/** 获取rsi12
	@return 返回rsi12
	*/
	BigNumber rsi12();

	/** 获取rsi24
	@return 返回rsi24
	*/
	BigNumber rsi24();

private:
	BigNumber m_rsi6;
	BigNumber m_rsi12;
	BigNumber m_rsi24;
};