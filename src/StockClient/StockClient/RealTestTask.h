#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include "StockSolution/StockSolutionAPI.h"

class StockClient;

class RealTestTask : public CTask
{
public:
	RealTestTask();

public:
	void DoTask();

	void setParam(SolutionEnum solutionEnum,
		StrategyEnum strategyEnum,
		const IntDateTime& beginTime,
		const IntDateTime& endTime,
		StockClient* stockClient);

private:
	SolutionEnum m_solutionEnum;
	StrategyEnum m_strategyEnum;
	IntDateTime m_beginTime;
	IntDateTime m_endTime;
	StockClient* m_stockClient;
};