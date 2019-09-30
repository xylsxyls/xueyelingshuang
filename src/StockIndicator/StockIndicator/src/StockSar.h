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
		/** ��ʼ��
		*/
		INIT = 0,

		/** ��
		*/
		RED = 1,

		/** ��
		*/
		GREEN = 2,

		/** ��ת��
		*/
		RED_TO_GREEN = 3,

		/** ��ת��
		*/
		GREEN_TO_RED = 4
	};
	BigNumber m_sar;
	SarState m_sarState;

	StockSar()
	{
		m_sarState = INIT;
	}
};