#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class StockClient;
class SaveAllMarketTask : public CTask
{
public:
	SaveAllMarketTask();

public:
	void DoTask();

	void setParam(int32_t beginIndex,
		StockClient* stockClient,
		const std::vector<std::string>& allStock = std::vector<std::string>());

protected:
	int32_t m_beginIndex;
	StockClient* m_stockClient;
	int32_t m_opposite;
	std::vector<std::string> m_allStock;
};