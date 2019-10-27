#include "SaveFilterStockTaskToMysql.h"
#include "StockMysql/StockMysqlAPI.h"
#include "StockClient.h"

SaveFilterStockTaskToMysql::SaveFilterStockTaskToMysql() :
m_today(0, 0),
m_stockClient(nullptr)
{

}

void SaveFilterStockTaskToMysql::DoTask()
{
	if (m_today.empty())
	{
		m_today = m_stockClient->m_today;
	}
	std::map<IntDateTime, std::vector<std::string>> filterStock;
	filterStock[m_today] = m_stockClient->m_allFilterStock;
	StockMysql::instance().saveFilterStockToMysql(filterStock);
}

void SaveFilterStockTaskToMysql::setParam(const IntDateTime& today, StockClient* stockClient)
{
	m_today = today;
	m_stockClient = stockClient;
}