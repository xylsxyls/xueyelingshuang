#include "StockEveryTestResultTask.h"

StockEveryTestResultTask::StockEveryTestResultTask():
m_allFund(nullptr),
m_allStockCount(nullptr)
{

}

void StockEveryTestResultTask::DoTask()
{
	*m_allFund = *m_allFund + m_fund;
	++(*m_allStockCount);
	RCSend("stock = %s, allFund = %s", m_stock.c_str(), m_fund.toString().c_str());
}

void StockEveryTestResultTask::setParam(const std::string& stock, const BigNumber& fund, BigNumber* allFund, BigNumber* allStockCount)
{
	m_stock = stock;
	m_fund = fund;
	m_allFund = allFund;
	m_allStockCount = allStockCount;
}