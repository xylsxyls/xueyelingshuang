#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include "StockSolution/StockSolutionAPI.h"

class StockClient;

class OnceTestTask : public CTask
{
public:
	OnceTestTask();

public:
	void DoTask();

	void setParam(SolutionType solutionType,
		std::vector<std::pair<StrategyType, StrategyType>>& vecStrategyType,
		const IntDateTime& beginTime,
		const IntDateTime& endTime,
		StockClient* stockClient);

private:
	SolutionType m_solutionType;
	std::vector<std::pair<StrategyType, StrategyType>> m_vecStrategyType;
	IntDateTime m_beginTime;
	IntDateTime m_endTime;
	StockClient* m_stockClient;
};