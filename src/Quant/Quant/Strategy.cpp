#include "Strategy.h"
#include <functional>
#include <unordered_map>

Strategy::Strategy() :
m_beginTime(0),
m_endTime(0),
m_mode(StrategyMode::COUNT)
{

}

Strategy::~Strategy()
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

std::vector<std::string> Strategy::getStock() const
{
	return m_vecStock;
}

void Strategy::setMarket(const std::shared_ptr<Market>& spMarket)
{
	m_spMarket = spMarket;
}

void Strategy::setFund(const std::shared_ptr<Fund>& spFund)
{
	m_spFund = spFund;
}

void Strategy::setStrategyParam(const std::vector<int32_t>& param)
{
	m_strategyParam = param;
}

std::vector<int32_t> Strategy::getStrategyParam() const
{
	return m_strategyParam;
}

bool Strategy::isStrategyParamValid() const
{
	return true;
}

StrategyMode Strategy::getStrategyMode() const
{
	return m_mode;
}

std::string Strategy::getStrategyName() const
{
	return m_modeName;
}