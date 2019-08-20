#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class StockClient;
class CheckAllStockTask : public CTask
{
public:
	CheckAllStockTask();

public:
	void DoTask();

	void setParam(StockClient* stockClient);

protected:
	//有问题的代码集合
	std::vector<std::string> m_vecErrorStock;
	StockClient* m_stockClient;
};