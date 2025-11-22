#pragma once

class StockManager
{
private:
	StockManager();

public:
	static StockManager& instance();

public:
	/** 初始化
	*/
	void init();

	/** 获取交易日列表
	@return 返回交易日列表
	*/
	std::vector<int32_t> getAllTradingDays();

private:
	// 交易日列表
	std::vector<int32_t> m_tradingDays;
};