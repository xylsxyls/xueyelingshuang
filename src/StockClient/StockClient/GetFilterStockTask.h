#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"

class StockClient;

class GetFilterStockTask : public CTask
{
public:
	GetFilterStockTask();

public:
	void DoTask();

	void setParam(HWND hWnd, const IntDateTime& today, bool regain, StockClient* stockClient);

private:
	HWND m_hWnd;
	IntDateTime m_today;
	bool m_regain;
	StockClient* m_stockClient;
};