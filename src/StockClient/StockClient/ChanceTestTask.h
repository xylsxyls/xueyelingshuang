#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include "StockSolution/StockSolutionAPI.h"

class StockClient;

class ChanceTestTask : public CTask
{
public:
	ChanceTestTask();

public:
	void DoTask();

	void setParam(SolutionType solutionType,
		std::vector<std::pair<StrategyType, StrategyType>>& vecStrategyType,
		const IntDateTime& beginTime,
		const IntDateTime& endTime,
		int32_t maxDay,
		StockClient* stockClient);

private:
	SolutionType m_solutionType;
	std::vector<std::pair<StrategyType, StrategyType>> m_vecStrategyType;
	IntDateTime m_beginTime;
	IntDateTime m_endTime;
	int32_t m_maxDay;
	StockClient* m_stockClient;
};