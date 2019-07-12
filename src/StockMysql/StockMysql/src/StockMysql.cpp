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

std::vector<std::string> StockMysql::allStock() const
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
	createMarketHead(stock);
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

std::vector<std::vector<std::string>> StockMysql::readMarket(const std::string& stock, const IntDateTime& beginTime, const IntDateTime& endTime) const
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
	return m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::selectString(stock, "*", SqlString::fieldStrToDate("date") + ">=" + SqlString::strToDate(useBeginTime.dateToString()) + " and " + SqlString::fieldStrToDate("date") + " <= " + SqlString::strToDate(useEndTime.dateToString()))))->toVector();
}

std::vector<std::vector<std::string>> StockMysql::readWr(const std::string& stock, const IntDateTime& beginTime, const IntDateTime& endTime) const
{
	m_mysql.selectDb("stockindex");
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
	return m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::selectString(stock, "date,wr10,wr20", SqlString::fieldStrToDate("date") + ">=" + SqlString::strToDate(useBeginTime.dateToString()) + " and " + SqlString::fieldStrToDate("date") + " <= " + SqlString::strToDate(useEndTime.dateToString()))))->toVector();
}

std::vector<std::vector<std::string>> StockMysql::readRsi(const std::string& stock, const IntDateTime& beginTime, const IntDateTime& endTime) const
{
	m_mysql.selectDb("stockindex");
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
	return m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::selectString(stock, "date,rsi6,rsi12,rsi24", SqlString::fieldStrToDate("date") + ">=" + SqlString::strToDate(useBeginTime.dateToString()) + " and " + SqlString::fieldStrToDate("date") + " <= " + SqlString::strToDate(useEndTime.dateToString()))))->toVector();
}

bool StockMysql::dateExist(const std::string& stock, const IntDateTime& date) const
{
	m_mysql.selectDb("stockmarket");
	return !(m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::selectString(stock, "*", SqlString::fieldStrToDate("date") + "=" + SqlString::strToDate(date.dateToString()))))->toVector().empty());
}

IntDateTime StockMysql::beginDate(const std::string& stock) const
{
	m_mysql.selectDb("stockmarket");
	std::string minSelect = SqlString::selectString(stock, SqlString::sqlMin(SqlString::fieldStrToDate("date")));
	auto vec = m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::selectString(stock, "date", SqlString::fieldStrToDate("date") + "=(" + minSelect + ")")))->toVector();
	if (vec.empty())
	{
		return IntDateTime(0, 0);
	}
	return vec[0][0];
}

IntDateTime StockMysql::endDate(const std::string& stock) const
{
	std::string maxSelect = SqlString::selectString(stock, SqlString::sqlMax(SqlString::fieldStrToDate("date")));
	auto vec = m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::selectString(stock, "date", SqlString::fieldStrToDate("date") + "=(" + maxSelect + ")")))->toVector();
	if (vec.empty())
	{
		return IntDateTime(0, 0);
	}
	return vec[0][0];
}

IntDateTime StockMysql::getDatePre(const std::string& stock, const IntDateTime& date) const
{
	m_mysql.selectDb("stockmarket");
	std::string maxSelect = SqlString::selectString(stock, SqlString::sqlMax(SqlString::fieldStrToDate("date")), SqlString::fieldStrToDate("date") + "<" + SqlString::strToDate(date.dateToString()));
	auto vec = m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::selectString(stock, "date", SqlString::fieldStrToDate("date") + "=(" + maxSelect + ")")))->toVector();
	if (vec.empty())
	{
		return IntDateTime(0, 0);
	}
	return vec[0][0];
}

IntDateTime StockMysql::getDateNext(const std::string& stock, const IntDateTime& date) const
{
	m_mysql.selectDb("stockmarket");
	std::string minSelect = SqlString::selectString(stock, SqlString::sqlMin(SqlString::fieldStrToDate("date")), SqlString::fieldStrToDate("date") + ">" + SqlString::strToDate(date.dateToString()));
	auto vec = m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::selectString(stock, "date", SqlString::fieldStrToDate("date") + "=(" + minSelect + ")")))->toVector();
	if (vec.empty())
	{
		return IntDateTime(0, 0);
	}
	return vec[0][0];
}

BigNumber StockMysql::getDays(const std::string& stock, const IntDateTime& date1, const IntDateTime& date2)
{
	m_mysql.selectDb("stockmarket");
	return atoi(m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::selectString(stock, "COUNT(*)", SqlString::fieldStrToDate("date") + ">=" + SqlString::strToDate(date1.dateToString()) + " and " + SqlString::fieldStrToDate("date") + " <= " + SqlString::strToDate(date2.dateToString()))))->toVector()[0][0].c_str());
}

//int main()
//{
//	IntDateTime ss(0, 0);
//	
//	auto sss1 = StockMysql::instance().getDatePre("603826", IntDateTime(20190430, 0));
//	auto sss2 = StockMysql::instance().getDateNext("603826", IntDateTime(20190430, 0));
//	auto sss3 = StockMysql::instance().dateExist("603826", IntDateTime(20190506, 0));
//	auto sss4 = StockMysql::instance().readMarket("603826", "2019-01-01", "2019-01-30");
//	auto sss5 = StockMysql::instance().beginDate("603826");
//	auto sss6 = StockMysql::instance().endDate("603826");
//	auto sss7 = StockMysql::instance().getDays("603826", "2019-04-27", "2019-05-06");
//	
//	return 0;
//}