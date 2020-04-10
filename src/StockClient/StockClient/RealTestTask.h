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

	void setParam(SolutionType solutionType,
		const std::vector<ChooseParam>& vecChooseParam,
		const IntDateTime& beginTime,
		const IntDateTime& endTime,
		StockClient* stockClient);

private:
	SolutionType m_solutionType;
	std::vector<ChooseParam> m_vecChooseParam;
	IntDateTime m_beginTime;
	IntDateTime m_endTime;
	StockClient* m_stockClient;
};