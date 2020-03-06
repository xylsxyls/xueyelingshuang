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
	BigNumber m_sar5;
	SarState m_sarState5;
	BigNumber m_sar10;
	SarState m_sarState10;
	BigNumber m_sar20;
	SarState m_sarState20;

	StockSar()
	{
		m_sarState5 = INIT;
		m_sarState10 = INIT;
		m_sarState20 = INIT;
		m_type = "sar";
	}
};