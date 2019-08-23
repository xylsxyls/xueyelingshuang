#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class StockClient;
class Win7SaveAllMarketTask : public CTask
{
public:
	Win7SaveAllMarketTask();

public:
	void DoTask();

	void setParam(int32_t beginIndex,
		int32_t endIndex,
		StockClient* stockClient,
		const std::vector<std::string>& allStock = std::vector<std::string>());

	void StopTask();

protected:
	int32_t m_beginIndex;
	int32_t m_end;
	StockClient* m_stockClient;
	std::vector<std::string> m_allStock;
	std::atomic<bool> m_exit;
};