#include "StockIndicator.h"
#include "StockWrIndicator.h"
#include "StockRsiIndicator.h"

StockIndicator::StockIndicator()
{

}

StockIndicator& StockIndicator::instance()
{
	static StockIndicator s_stockIndicator;
	return s_stockIndicator;
}

std::shared_ptr<StockWrIndicator> StockIndicator::wr(const std::string& stock,
	const IntDateTime& beginTime, 
	const IntDateTime& endTime)
{
	std::shared_ptr<StockWrIndicator> spStockWrIndicator(new StockWrIndicator);
	spStockWrIndicator->load(stock, beginTime, endTime);
	return spStockWrIndicator;
}

std::shared_ptr<StockRsiIndicator> StockIndicator::rsi(const std::string& stock,
	const IntDateTime& beginTime,
	const IntDateTime& endTime)
{
	std::shared_ptr<StockRsiIndicator> spStockRsiIndicator(new StockRsiIndicator);
	spStockRsiIndicator->load(stock, beginTime, endTime);
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