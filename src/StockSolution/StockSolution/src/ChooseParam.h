#pragma once
#include "StockStrategy/StockStrategyAPI.h"
#include "StockSolutionMacro.h"

enum SolutionType
{
	SOLUTION_INIT,

	AVG_FUND_HIGH_SCORE,

	STRATEGY_SET,

	DISPOSABLE_STRATEGY,

	INTEGRATED_STRATEGY,

	OBSERVE_STRATEGY
};

struct StockSolutionAPI ChooseParam
{
	//ʹ�õ�����
	StrategyType m_useType;
	//ʹ�õļ�������
	StrategyType m_useCountType;
	//�Ƿ��ǹ۲�����
	bool m_isObserve;
	//����������ͣ������observer�����ڲ����ͣ�������ֱ��ʹ��
	SolutionType m_solutionType;

	ChooseParam()
	{
		m_useType = STRATEGY_INIT;
		m_useCountType = STRATEGY_INIT;
		m_isObserve = false;
		m_solutionType = SOLUTION_INIT;
	}
};