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
	int32_t lineIndex = -1;
	while (lineIndex++ != m_stockLog.size() - 1)
	{
		RCSend("stocklog = %s", m_stockLog[lineIndex].c_str());
	}
	RCSend("stock = %s, allFund = %s", m_stock.c_str(), m_fund.toString().c_str());
}

void StockEveryTestResultTask::setParam(const std::string& stock, const std::vector<std::string>& stockLog, const BigNumber& fund, BigNumber* allFund, BigNumber* allStockCount)
{
	m_stock = stock;
	m_stockLog = stockLog;
	m_fund = fund;
	m_allFund = allFund;
	m_allStockCount = allStockCount;
}