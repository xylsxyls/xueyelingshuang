#pragma once
#include "IntDateTime/IntDateTimeAPI.h"
#include "StockTradeMacro.h"

class StockFund;
struct StockTradeAPI TradeParam
{
	//ִ�е�zhanghu
	StockFund* m_stockFund;
	//ÿ�ο�ʼ��ʱ��
	IntDateTime m_onceDate;

	TradeParam():
		m_onceDate(0, 0)
	{
		m_stockFund = nullptr;
	}
};