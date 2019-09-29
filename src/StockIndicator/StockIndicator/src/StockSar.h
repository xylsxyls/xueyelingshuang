#pragma once
#include "IndicatorBase.h"
#include "StockIndicatorMacro.h"

/** sarÊý¾Ýzhibiao
*/
struct StockIndicatorAPI StockSar : public IndicatorBase
{
	/** sar×´Ì¬
	*/
	enum SarState
	{
		/** ºì
		*/
		RED,

		/** ÂÌ
		*/
		GREEN,

		/** ºì×ªÂÌ
		*/
		RED_TO_GREEN,

		/** ÂÌ×ªºì
		*/
		GREEN_TO_RED
	};
	BigNumber m_sar;
	SarState m_sarState;
};