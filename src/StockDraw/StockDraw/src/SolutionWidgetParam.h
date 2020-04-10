#pragma once
#include "IntDateTime/IntDateTimeAPI.h"
#include <QMetaType>
#include "BigNumber/BigNumberAPI.h"
#include "StockDrawMacro.h"
#include "StockStrategy/StockStrategyAPI.h"

class StockFund;

struct StockDrawAPI SolutionWidgetParam
{
	IntDateTime m_date;
	std::vector<std::pair<std::string, StockInfo>> m_buyStock;
	std::vector<std::pair<std::string, StockInfo>> m_sellStock;
	StockFund* m_stockFund;

	SolutionWidgetParam()
	{
		m_stockFund = nullptr;
		m_date.clear();
	}
};
Q_DECLARE_METATYPE(SolutionWidgetParam);