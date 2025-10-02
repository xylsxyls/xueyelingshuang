#include "Strategy.h"
#include <functional>
#include <unordered_map>

Strategy::Strategy()
	: m_beginTime(0)
	, m_endTime(0)
{
}

void Strategy::init(uint32_t beginTime, uint32_t endTime)
{
	m_beginTime = beginTime;
	m_endTime = endTime;
}

void Strategy::addStock(const std::string& stock)
{
	m_vecStock.push_back(stock);
}

void Strategy::setMarket(const std::shared_ptr<Market>& spMarket)
{
	m_spMarket = spMarket;
}

void Strategy::setFund(const std::shared_ptr<Fund>& spFund)
{
	m_spFund = spFund;
}

void Strategy::setStrategyParams(const std::vector<int32_t>& params)
{
	m_strategyParams = params;
}

std::vector<int32_t> Strategy::getStrategyParams() const
{
	return m_strategyParams;
}

const std::vector<std::string>& Strategy::getStocks() const
{
	return m_vecStock;
}