#pragma once
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class StockMarket;
class StockWrIndicator;
class StockRsiIndicator;
class StockClient;

class EveryTestTask : public CTask
{
public:
	EveryTestTask();

	void DoTask();

	void setParam(const std::string& stock,
		const std::shared_ptr<StockMarket>& spMarket,
		const std::shared_ptr<StockWrIndicator>& spStockWrIndicator,
		const std::shared_ptr<StockRsiIndicator>& spStockRsiIndicator,
		StockClient* stockClient);

private:
	std::string m_stock;
	std::shared_ptr<StockMarket> m_spMarket;
	std::shared_ptr<StockWrIndicator> m_spStockWrIndicator;
	std::shared_ptr<StockRsiIndicator> m_spStockRsiIndicator;
	StockClient* m_stockClient;
};