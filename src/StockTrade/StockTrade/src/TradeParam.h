#pragma once
#include "IntDateTime/IntDateTimeAPI.h"
#include "StockTradeMacro.h"

class StockFund;
struct StockTradeAPI TradeParam
{
	//执行的zhanghu
	StockFund* m_stockFund;
	//每次开始的时间
	IntDateTime m_onceDate;

	TradeParam():
		m_onceDate(0, 0)
	{
		m_stockFund = nullptr;
	}
};