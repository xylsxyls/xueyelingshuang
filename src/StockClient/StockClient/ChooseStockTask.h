#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "StockStrategy/StockStrategyAPI.h"
#include "StockSolution/StockSolutionAPI.h"

class StockClient;
class StockFund;

class ChooseStockTask : public CTask
{
public:
	ChooseStockTask();

public:
	void DoTask();

	void setParam(const IntDateTime& date,
		const std::vector<std::string>& allStock,
		const std::vector<ChooseParam>& vecChooseParam,
		SolutionType solutionType,
		StockFund* stockFund,
		StockClient* stockClient);

private:
	IntDateTime m_date;
	std::vector<std::string> m_allStock;
	std::vector<ChooseParam> m_vecChooseParam;
	SolutionType m_solutionType;
	StockFund* m_stockFund;
	StockClient* m_stockClient;
};