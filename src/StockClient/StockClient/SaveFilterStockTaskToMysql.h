#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"

class StockClient;

class SaveFilterStockTaskToMysql : public CTask
{
public:
	SaveFilterStockTaskToMysql();

public:
	void DoTask();

	void setParam(const IntDateTime& today, StockClient* stockClient);

private:
	IntDateTime m_today;
	StockClient* m_stockClient;
};