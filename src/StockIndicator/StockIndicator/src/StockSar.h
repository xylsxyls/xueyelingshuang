#pragma once
#include "IndicatorBase.h"
#include "StockIndicatorMacro.h"

/** sar数据zhibiao
*/
struct StockIndicatorAPI StockSar : public IndicatorBase
{
	/** sar状态
	*/
	enum SarState
	{
		/** 初始化
		*/
		INIT = 0,

		/** 红
		*/
		RED = 1,

		/** 绿
		*/
		GREEN = 2,

		/** 红转绿
		*/
		RED_TO_GREEN = 3,

		/** 绿转红
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