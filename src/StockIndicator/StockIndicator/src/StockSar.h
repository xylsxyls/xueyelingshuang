#pragma once
#include "IndicatorBase.h"
#include "StockIndicatorMacro.h"

/** sar����zhibiao
*/
struct StockIndicatorAPI StockSar : public IndicatorBase
{
	/** sar״̬
	*/
	enum SarState
	{
		/** ��
		*/
		RED,

		/** ��
		*/
		GREEN,

		/** ��ת��
		*/
		RED_TO_GREEN,

		/** ��ת��
		*/
		GREEN_TO_RED
	};
	BigNumber m_sar;
	SarState m_sarState;
};