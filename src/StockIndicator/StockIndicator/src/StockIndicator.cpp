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
	m_redisIndicatorData = StockMysql::instance().readIndicator(stock, beginTime, endTime);
}

void StockIndicator::clear()
{
	m_redisIndicatorData.clear();
}

std::shared_ptr<StockWrIndicator> StockIndicator::wr(const std::string& stock,
	const IntDateTime& beginTime, 
	const IntDateTime& endTime)
{
	std::vector<std::vector<std::string>> redisData;
	int32_t lineIndex = -1;
	while (lineIndex++ != m_redisIndicatorData.size() - 1)
	{
		redisData.push_back(std::vector<std::string>());
		const std::vector<std::string>& vecLine = m_redisIndicatorData[lineIndex];
		redisData.back().push_back(vecLine[0]);
		redisData.back().push_back(vecLine[1]);
		redisData.back().push_back(vecLine[2]);
	}

	std::shared_ptr<StockWrIndicator> spStockWrIndicator(new StockWrIndicator);
	spStockWrIndicator->setRedisData(stock, redisData);
	return spStockWrIndicator;
}

std::shared_ptr<StockRsiIndicator> StockIndicator::rsi(const std::string& stock,
	const IntDateTime& beginTime,
	const IntDateTime& endTime)
{
	std::vector<std::vector<std::string>> redisData;
	int32_t lineIndex = -1;
	while (lineIndex++ != m_redisIndicatorData.size() - 1)
	{
		redisData.push_back(std::vector<std::string>());
		const std::vector<std::string>& vecLine = m_redisIndicatorData[lineIndex];
		redisData.back().push_back(vecLine[0]);
		redisData.back().push_back(vecLine[3]);
		redisData.back().push_back(vecLine[4]);
		redisData.back().push_back(vecLine[5]);
	}

	std::shared_ptr<StockRsiIndicator> spStockRsiIndicator(new StockRsiIndicator);
	spStockRsiIndicator->setRedisData(stock, redisData);
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