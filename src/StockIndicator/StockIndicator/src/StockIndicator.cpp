#include "StockIndicator.h"
#include "StockWrIndicator.h"
#include "StockRsiIndicator.h"
#include "StockMysql/StockMysqlAPI.h"

StockIndicator::StockIndicator()
{

}

StockIndicator& StockIndicator::instance()
{
	static StockIndicator s_stockIndicator;
	return s_stockIndicator;
}

void StockIndicator::loadFromRedis(const std::string& stock, const IntDateTime& beginTime, const IntDateTime& endTime)
{
	m_indicatorIndex = StockMysql::instance().getIndicatorDataIndex();
	m_loadStock = stock;
	m_redisIndicatorData = StockMysql::instance().readIndicator(stock, beginTime, endTime);
}

void StockIndicator::clear()
{
	m_indicatorIndex.clear();
	m_loadStock.clear();
	m_redisIndicatorData = nullptr;
}

std::shared_ptr<StockWrIndicator> StockIndicator::wr()
{
	const std::vector<int32_t>& indicatorIndex = m_indicatorIndex.find("wr")->second;
	int32_t dateIndex = m_indicatorIndex.find("date")->second[0];
	std::shared_ptr<StockWrIndicator> spStockWrIndicator(new StockWrIndicator);
	spStockWrIndicator->setRedisData(m_loadStock, m_redisIndicatorData, dateIndex, indicatorIndex);
	return spStockWrIndicator;
}

std::shared_ptr<StockRsiIndicator> StockIndicator::rsi()
{
	const std::vector<int32_t>& indicatorIndex = m_indicatorIndex.find("rsi")->second;
	int32_t dateIndex = m_indicatorIndex.find("date")->second[0];
	std::shared_ptr<StockRsiIndicator> spStockRsiIndicator(new StockRsiIndicator);
	spStockRsiIndicator->setRedisData(m_loadStock, m_redisIndicatorData, dateIndex, indicatorIndex);
	return spStockRsiIndicator;
}

//int main()
//{
//	StockWrIndicator in;
//	in.load("603826", "2019-01-01", "2019-04-30");
//
//	StockRsiIndicator in2;
//	in2.load("603826", "2019-01-01", "2019-04-30");
//	return 0;
//}