#include "StockMysql.h"
#include "MysqlCpp/MysqlCppAPI.h"
#include "CStringManager/CStringManagerAPI.h"

StockMysql::StockMysql()
{
	init();
}

StockMysql& StockMysql::instance()
{
	static StockMysql s_stockMysql;
	return s_stockMysql;
}

void StockMysql::init()
{
	if (!m_mysql.connect("127.0.0.1", 3306, "root", ""))
	{
		printf("数据库连接失败\n");
		system("pause");
	}
}

std::vector<std::string> StockMysql::allStock()
{
	m_mysql.selectDb("stockmarket");
	std::vector<std::string> result;
	auto allStock = m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::selectString("stock")))->toVector();
	int32_t index = -1;
	while (index++ != allStock.size() - 1)
	{
		result.push_back(allStock[index][0]);
	}
	return result;
}

void StockMysql::createMarketHead(const std::string& stock)
{
	m_mysql.selectDb("stockmarket");
	std::vector<std::string> vecFields;
	vecFields.push_back("date varchar(10)");
	vecFields.push_back("open varchar(7)");
	vecFields.push_back("high varchar(7)");
	vecFields.push_back("low varchar(7)");
	vecFields.push_back("close varchar(7)");
	m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::createTableString(stock, vecFields)));
}

void StockMysql::addStock(const std::string& stock)
{
	m_mysql.selectDb("stockmarket");
	auto prepare = m_mysql.PreparedStatementCreator(SqlString::insertString("stock", "stock,name"));
	prepare->setString(0, stock);
	prepare->setString(1, "");
	m_mysql.execute(prepare);
}

void StockMysql::saveMarket(const std::string& stock, const std::vector<std::vector<std::string>>& vecMarket)
{
	m_mysql.selectDb("stockmarket");
	int32_t lineIndex = -1;
	while (lineIndex++ != vecMarket.size() - 1)
	{
		auto prepare = m_mysql.PreparedStatementCreator(SqlString::insertString(stock, "date,open,high,low,close"));
		int32_t index = -1;
		while (index++ != vecMarket[lineIndex].size() - 1)
		{
			prepare->setString(index, vecMarket[lineIndex][index]);
		}
		m_mysql.execute(prepare);
	}
}

std::vector<std::vector<std::string>> StockMysql::readMarket(const std::string& stock, const IntDateTime& beginTime, const IntDateTime& endTime)
{
	m_mysql.selectDb("stockmarket");
	IntDateTime useBeginTime = beginTime;
	IntDateTime useEndTime = endTime;
	if (beginTime.empty())
	{
		useBeginTime.setTime(19700101, 0);
	}
	if (endTime.empty())
	{
		useEndTime.setTime(31500101, 0);
	}
	return m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::selectString(stock, "*", "STR_TO_DATE(date, '%Y-%m-%d') >= STR_TO_DATE('" + useBeginTime.dateToString() + "', '%Y-%m-%d') and STR_TO_DATE(date, '%Y-%m-%d') <= STR_TO_DATE('" + useEndTime.dateToString() + "', '%Y-%m-%d')")))->toVector();
}

//int main()
//{
//	IntDateTime ss(0, 0);
//	
//	auto sss = StockMysql::instance().readMarket("603826", IntDateTime(20190430, 0));
//	
//	return 0;
//}