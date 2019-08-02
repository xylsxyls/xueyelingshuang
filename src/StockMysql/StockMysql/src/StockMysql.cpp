#include "StockMysql.h"
#include "MysqlCpp/MysqlCppAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "hiredisinclude/hiredis.h"

StockMysql::StockMysql()
{
	init();
}

std::shared_ptr<StockMysql> StockMysql::newCase()
{
	return std::shared_ptr<StockMysql>(new StockMysql);
}

void StockMysql::init()
{
	if (!m_mysql.connect("127.0.0.1", 3306, "root", ""))
	{
		::MessageBox(nullptr, "数据库连接失败\n", nullptr, 0);
		system("pause");
	}
	if (!m_redis.connect("127.0.0.1"))
	{
		::MessageBox(nullptr, "redis连接失败\n", nullptr, 0);
		system("pause");
	}
}

std::vector<std::string> StockMysql::allStock() const
{
	m_redis.selectDbIndex(0);
	if (!m_redis.keyExist("stock"))
	{
		m_mysql.selectDb("stockmarket");
		std::vector<std::string> group;
		auto allStock = m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::selectString("stock")))->toVector();
		int32_t index = -1;
		while (index++ != allStock.size() - 1)
		{
			group.push_back(allStock[index][0]);
		}
		m_redis.setGroups("stock", group);
		return group;
	}
	return m_redis.getGroup("stock")->toGroup();
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

//std::vector<std::vector<std::string>> StockMysql::readMarket(const std::string& stock, const IntDateTime& beginTime, const IntDateTime& endTime) const
//{
//	IntDateTime useBeginTime = beginTime;
//	IntDateTime useEndTime = endTime;
//	if (beginTime.empty())
//	{
//		useBeginTime.setTime(19700101, 0);
//	}
//	if (endTime.empty())
//	{
//		useEndTime.setTime(31500101, 0);
//	}
//	std::vector<std::vector<std::string>> result;
//	m_redis.selectDbIndex(0);
//	std::vector<std::string> stockDate = m_redis.getOrderGroupByScore(stock, useBeginTime.getDate(), useEndTime.getDate())->toGroup();
//	if (stockDate.empty())
//	{
//		m_mysql.selectDb("stockmarket");
//		result = m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::selectString(stock, "*", SqlString::fieldStrToDate("date") + ">=" + SqlString::strToDate(useBeginTime.dateToString()) + " and " + SqlString::fieldStrToDate("date") + " <= " + SqlString::strToDate(useEndTime.dateToString()))))->toVector();
//
//		std::vector<std::pair<int32_t, std::string>> dateGroup;
//		std::map<std::string, std::string> openHashMap;
//		std::map<std::string, std::string> highHashMap;
//		std::map<std::string, std::string> lowHashMap;
//		std::map<std::string, std::string> closeHashMap;
//		int32_t lineIndex = -1;
//		while (lineIndex++ != result.size() - 1)
//		{
//			const std::vector<std::string>& vecLine = result[lineIndex];
//			dateGroup.push_back(std::pair<int32_t, std::string>(IntDateTime(vecLine[0]).getDate(), vecLine[0]));
//			openHashMap[vecLine[0]] = vecLine[1];
//			highHashMap[vecLine[0]] = vecLine[2];
//			lowHashMap[vecLine[0]] = vecLine[3];
//			closeHashMap[vecLine[0]] = vecLine[4];
//		}
//		m_redis.setOrderGroups(stock, dateGroup);
//		m_redis.selectDbIndex(1);
//		m_redis.setHashMap(stock, openHashMap);
//		m_redis.selectDbIndex(2);
//		m_redis.setHashMap(stock, highHashMap);
//		m_redis.selectDbIndex(3);
//		m_redis.setHashMap(stock, lowHashMap);
//		m_redis.selectDbIndex(4);
//		m_redis.setHashMap(stock, closeHashMap);
//		return result;
//	}
//	m_redis.selectDbIndex(1);
//	std::vector<std::string> vecOpen = m_redis.getHashValues(stock, stockDate)->toGroup();
//	m_redis.selectDbIndex(2);
//	std::vector<std::string> vecHigh = m_redis.getHashValues(stock, stockDate)->toGroup();
//	m_redis.selectDbIndex(3);
//	std::vector<std::string> vecLow = m_redis.getHashValues(stock, stockDate)->toGroup();
//	m_redis.selectDbIndex(4);
//	std::vector<std::string> vecClose = m_redis.getHashValues(stock, stockDate)->toGroup();
//
//	std::vector<std::string> vecLine;
//	int32_t index = -1;
//	while (index++ != vecClose.size() - 1)
//	{
//		vecLine.clear();
//		vecLine.push_back(stockDate[index]);
//		vecLine.push_back(vecOpen[index]);
//		vecLine.push_back(vecHigh[index]);
//		vecLine.push_back(vecLow[index]);
//		vecLine.push_back(vecClose[index]);
//		result.push_back(vecLine);
//	}
//	return result;
//}

std::vector<std::vector<std::string>> StockMysql::readMarket(const std::string& stock, const IntDateTime& beginTime, const IntDateTime& endTime) const
{
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
	std::vector<std::vector<std::string>> result;
	m_redis.selectDbIndex(0);
	auto spResultSet = m_redis.getOrderGroupByScore(stock, useBeginTime.getDate(), useEndTime.getDate());
	if (spResultSet->toCount() == 0)
	{
		if (!m_redis.getOrderGroupByScore(stock, 0, 0)->toGroup().empty())
		{
			return result;
		}
		m_mysql.selectDb("stockmarket");
		result = m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::selectString(stock, "*", SqlString::fieldStrToDate("date") + ">=" + SqlString::strToDate(IntDateTime("1970-01-01").dateToString()) + " and " + SqlString::fieldStrToDate("date") + " <= " + SqlString::strToDate(IntDateTime("3150-01-01").dateToString()))))->toVector();

		std::vector<std::pair<int32_t, std::string>> orderGroup;
		orderGroup.push_back(std::pair<int32_t, std::string>(0, stock));
		int32_t lineIndex = -1;
		while (lineIndex++ != result.size() - 1)
		{
			std::string strLine;
			const std::vector<std::string>& vecLine = result[lineIndex];
			int32_t index = -1;
			while (index++ != vecLine.size() - 2)
			{
				strLine.append(vecLine[index] + ",");
			}
			strLine.append(vecLine[index]);
			orderGroup.push_back(std::pair<int32_t, std::string>(IntDateTime(vecLine[0]).getDate(), strLine));
		}
		if (!m_redis.setOrderGroups(stock, orderGroup))
		{
			RCSend("setOrderGroups error = %s", stock.c_str());
		}
		return result;
	}
	std::vector<std::string> vecLine;
	int32_t index = -1;
	while (index++ != spResultSet->toReply()->elements - 1)
	{
		CStringManager::split(vecLine, spResultSet->toReply()->element[index]->str, ',');
		result.push_back(vecLine);
	}
	return result;
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

//int main()
//{
//	IntDateTime ss(0, 0);
//	
//	RCSend("begin = %d", ::GetTickCount());
//
//	//MysqlCpp mysql;
//	//mysql.connect("127.0.0.1", 3306, "root", "");
//	//mysql.selectDb("stockmarket");
//	IntDateTime beginTime = "2015-01-01";
//	IntDateTime endTime = "2019-04-30";
//	std::shared_ptr<StockMysql> db = StockMysql::newCase();
//	std::vector<std::string> vecAllStock = db->allStock();
//	int32_t index = -1;
//	while (index++ != vecAllStock.size() - 1)
//	{
//		//auto result = mysql.execute(mysql.PreparedStatementCreator(SqlString::selectString(vecAllStock[index], "*", SqlString::fieldStrToDate("date") + ">=" + SqlString::strToDate(IntDateTime("1970-01-01").dateToString()) + " and " + SqlString::fieldStrToDate("date") + " <= " + SqlString::strToDate(IntDateTime("3150-01-01").dateToString()))))->toVector();
//		//vecAllStock[index]
//		auto sss1 = db->readMarket(vecAllStock[index], beginTime, endTime);
//		//if (result != sss1)
//		//{
//		//	RCSend("error compare = %s", vecAllStock[index].c_str());
//		//}
//		RCSend("%d", index + 1);
//	}
//	RCSend("end = %d", ::GetTickCount());
//	getchar();
//	return 0;
//}