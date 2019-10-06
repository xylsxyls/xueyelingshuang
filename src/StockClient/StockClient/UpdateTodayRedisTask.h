#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class StockClient;

class UpdateTodayRedisTask : public CTask
{
public:
	UpdateTodayRedisTask();

public:
	void DoTask();

	void setParam(const std::vector<std::string>& allUpdateStock, bool useLast, StockClient* stockClient);

private:
	std::vector<std::string> m_allUpdateStock;
	bool m_useLast;
	StockClient* m_stockClient;
};