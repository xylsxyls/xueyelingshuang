#pragma once
#include "StockStrategy/StockStrategyAPI.h"
#include "StockSolutionMacro.h"

struct StockSolutionAPI ChooseParam
{
	//ʹ�õ�����
	StrategyType m_useType;
	//ʹ�õļ�������
	StrategyType m_useCountType;
	//�Ƿ��ǹ۲�����
	bool m_isObserve;

	ChooseParam()
	{
		m_useType = STRATEGY_INIT;
		m_useCountType = STRATEGY_INIT;
		m_isObserve = false;
	}
};