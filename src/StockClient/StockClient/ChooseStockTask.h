#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "StockStrategy/StockStrategyAPI.h"

class StockClient;

class ChooseStockTask : public CTask
{
public:
	ChooseStockTask();

public:
	void DoTask();

	void setParam(const IntDateTime& date,
		const std::vector<std::string>& allStock,
		StrategyEnum strategyEnum,
		StockClient* stockClient);

private:
	IntDateTime m_date;
	std::vector<std::string> m_allStock;
	StrategyEnum m_strategyEnum;
	StockClient* m_stockClient;
};