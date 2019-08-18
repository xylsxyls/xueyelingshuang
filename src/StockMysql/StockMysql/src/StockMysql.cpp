#include "StockMysql.h"
#include "MysqlCpp/MysqlCppAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "hiredisinclude/hiredis.h"
#include "CSystem/CSystemAPI.h"

StockMysql::StockMysql()
{
	init();
}

std::shared_ptr<StockMysql> StockMysql::newCase()
{
	return std::shared_ptr<StockMysql>(new StockMysql);
}

StockMysql& StockMysql::instance()
{
	static StockMysql s_stockMysql;
	return s_stockMysql;
}

void StockMysql::init()
{
	if (CSystem::GetSystemVersionNum() < 655360)
	{
		if (!m_mysql.connect("192.168.1.2", 3306, "root", ""))
		{
			::MessageBox(nullptr, "数据库连接失败\n", nullptr, 0);
			system("pause");
		}
	}
	if (!CSystem::processPid(L"redis-server.exe").empty())
	{
		if (!m_redis.connect("127.0.0.1"))
		{
			::MessageBox(nullptr, "redis连接失败\n", nullptr, 0);
			system("pause");
		}
	}

	if (!CSystem::processPid(L"mysqld.exe").empty())
	{
		if (!m_mysql.connect("127.0.0.1", 3306, "root", ""))
		{
			::MessageBox(nullptr, "数据库连接失败\n", nullptr, 0);
			system("pause");
		}
	}
}

std::vector<std::string> StockMysql::allStock() const
{
	m_redis.selectDbIndex(0);
	if (!m_redis.keyExist("stock"))
	{
		std::vector<std::string> group = allStockFromMysql();
		m_redis.setGroups("stock", group);
		return group;
	}
	return m_redis.getGroup("stock")->toGroup();
}

std::vector<std::string> StockMysql::allStockFromMysql() const
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

std::shared_ptr<std::vector<std::vector<std::string>>> StockMysql::readMarket(const std::string& stock,
	const IntDateTime& beginTime,
	const IntDateTime& endTime) const
{
	std::vector<std::string> vecDbName;
	std::vector<std::string> vecDbField;
	vecDbName.push_back("stockmarket");
	vecDbField.push_back("*");
	return redisFromMysql(stock, beginTime, endTime, 0, vecDbName, vecDbField);
}

std::shared_ptr<std::vector<std::vector<std::string>>> StockMysql::readIndicator(const std::string& stock,
	const IntDateTime& beginTime,
	const IntDateTime& endTime) const
{
	std::vector<std::string> vecDbName;
	std::vector<std::string> vecDbField;
	vecDbName.push_back("stockindex");
	vecDbName.push_back("stockindex");
	vecDbField.push_back("date,wr10,wr20");
	vecDbField.push_back("rsi6,rsi12,rsi24");
	return redisFromMysql(stock, beginTime, endTime, 1, vecDbName, vecDbField);
}

void StockMysql::saveMarketDataIndex() const
{
	m_redis.selectDbIndex(0);
	std::map<std::string, std::string> hashMap;
	hashMap["date"] = "0";
	hashMap["market"] = "1,2,3,4";
	m_redis.setHashMap("marketdataindex", hashMap);
}

std::map<std::string, std::vector<int32_t>> StockMysql::getMarketDataIndex() const
{
	return getIndex("marketdataindex");
}

void StockMysql::saveIndicatorDataIndex() const
{
	m_redis.selectDbIndex(0);
	std::map<std::string, std::string> hashMap;
	hashMap["date"] = "0";
	hashMap["wr"] = "1,2";
	hashMap["rsi"] = "3,4,5";
	m_redis.setHashMap("indicatordataindex", hashMap);
}

std::map<std::string, std::vector<int32_t>> StockMysql::getIndicatorDataIndex() const
{
	return getIndex("indicatordataindex");
}

void StockMysql::saveAllDataIndex() const
{
	m_redis.selectDbIndex(0);
	std::map<std::string, std::string> hashMap;
	hashMap["date"] = "0";
	hashMap["market"] = "1,2,3,4";
	hashMap["wr"] = "5,6";
	hashMap["rsi"] = "7,8,9";
	m_redis.setHashMap("alldataindex", hashMap);
}

std::map<std::string, std::vector<int32_t>> StockMysql::getAllDataIndex() const
{
	return getIndex("alldataindex");
}

std::shared_ptr<std::vector<std::vector<std::string>>> StockMysql::readAll(const std::string& stock, const IntDateTime& beginTime /*= IntDateTime(0, 0)*/, const IntDateTime& endTime /*= IntDateTime(0, 0)*/) const
{
	std::vector<std::string> vecDbName;
	std::vector<std::string> vecDbField;
	vecDbName.push_back("stockmarket");
	vecDbName.push_back("stockindex");
	vecDbName.push_back("stockindex");
	vecDbField.push_back("*");
	vecDbField.push_back("wr10,wr20");
	vecDbField.push_back("rsi6,rsi12,rsi24");
	return redisFromMysql(stock, beginTime, endTime, 2, vecDbName, vecDbField);
}

bool StockMysql::dateExist(const std::string& stock, const IntDateTime& date) const
{
	return m_redis.getOrderGroupByScore(stock, date.getDate(), date.getDate())->toGroup().size() == 1;
}

IntDateTime StockMysql::beginDate(const std::string& stock) const
{
	std::vector<std::string> firstDate = m_redis.getOrderGroupByIndex(stock, 1, 1)->toGroup();
	if (firstDate.empty())
	{
		return IntDateTime(0, 0);
	}
	return IntDateTime(*firstDate.begin());
}

IntDateTime StockMysql::endDate(const std::string& stock) const
{
	int32_t count = (int32_t)m_redis.getOrderGroupCount(stock);
	if (count <= 1)
	{
		return IntDateTime(0, 0);
	}
	std::vector<std::string> endDate = m_redis.getOrderGroupByIndex(stock, count - 1, count - 1)->toGroup();
	if (endDate.empty())
	{
		return IntDateTime(0, 0);
	}
	return IntDateTime(*endDate.begin());
}

IntDateTime StockMysql::getDatePre(const std::string& stock, const IntDateTime& date) const
{
	int32_t count = (int32_t)m_redis.getOrderGroupCount(stock);
	if (count <= 1)
	{
		return IntDateTime(0, 0);
	}
	
	auto dayData = m_redis.getOrderGroupByScore(stock, date.getDate(), date.getDate())->toGroup();
	int32_t datePreCount = (int32_t)m_redis.getOrderGroupCountByScore(stock, 19700101, date.getDate());
	if (dayData.empty())
	{
		if (datePreCount == 0)
		{
			return IntDateTime(0, 0);
		}
		return IntDateTime(m_redis.getOrderGroupByIndex(stock, datePreCount, datePreCount)->toGroup()[0]);
	}
	if (datePreCount <= 1)
	{
		return IntDateTime(0, 0);
	}
	return IntDateTime(m_redis.getOrderGroupByIndex(stock, datePreCount - 1, datePreCount - 1)->toGroup()[0]);
}

IntDateTime StockMysql::getDateNext(const std::string& stock, const IntDateTime& date) const
{
	int32_t count = (int32_t)m_redis.getOrderGroupCount(stock);
	if (count <= 1)
	{
		return IntDateTime(0, 0);
	}

	IntDateTime lastDate = endDate(stock);
	if (date >= lastDate)
	{
		return IntDateTime(0, 0);
	}
	auto dayData = m_redis.getOrderGroupByScore(stock, date.getDate(), date.getDate())->toGroup();
	int32_t dateNextCount = (int32_t)m_redis.getOrderGroupCountByScore(stock, date.getDate(), lastDate.getDate());
	if (dayData.empty())
	{
		return IntDateTime(m_redis.getOrderGroupByIndex(stock, count - dateNextCount, count - dateNextCount)->toGroup()[0]);
	}
	return IntDateTime(m_redis.getOrderGroupByIndex(stock, count - dateNextCount + 1, count - dateNextCount + 1)->toGroup()[0]);
}

BigNumber StockMysql::getDays(const std::string& stock, const IntDateTime& dateBegin, const IntDateTime& dateEnd) const
{
	return (int32_t)m_redis.getOrderGroupCountByScore(stock, dateBegin.getDate(), dateEnd.getDate());
}

void StockMysql::initRedis()
{
	int32_t begin = ::GetTickCount();
	std::vector<std::string> vecAllStock = allStock();
	int32_t index = -1;
	while (index++ != vecAllStock.size() - 1)
	{
		auto sss4 = readMarket(vecAllStock[index]);
		auto sss5 = readIndicator(vecAllStock[index]);
		auto sss3 = readAll(vecAllStock[index]);
		RCSend("%d", index + 1);
	}
	saveMarketDataIndex();
	saveIndicatorDataIndex();
	saveAllDataIndex();
	RCSend("init end time = %.2lf分", (::GetTickCount() - begin) / 60000.0);
}

void StockMysql::saveAllStock(const std::vector<std::vector<std::string>>& allStock)
{
	m_mysql.selectDb("stockmarket");
	m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::destroyTableString("stock")));
	std::vector<std::string> vecFields;
	vecFields.push_back("stock varchar(6) primary key");
	vecFields.push_back("name varchar(63)");
	vecFields.push_back("isST varchar(1)");
	m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::createTableString("stock", vecFields)), false);

	int32_t index = -1;
	while (index++ != allStock.size() - 1)
	{
		const std::string& stock = allStock[index][0];
		const std::string& name = allStock[index][1];
		const std::string& isST = allStock[index][2];
		auto prepare = m_mysql.PreparedStatementCreator(SqlString::insertString("stock", "stock,name,isST"));
		prepare->setString(0, stock);
		prepare->setString(1, name);
		prepare->setString(2, isST);
		m_mysql.execute(prepare, false);
	}
	m_mysql.commit();
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

std::shared_ptr<std::vector<std::vector<std::string>>> StockMysql::redisFromMysql(const std::string& stock,
	const IntDateTime& beginTime,
	const IntDateTime& endTime,
	int32_t redisDbIndex,
	const std::vector<std::string>& mysqlDbName,
	const std::vector<std::string>& mysqlFields) const
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
	std::shared_ptr<std::vector<std::vector<std::string>>> result(new std::vector<std::vector<std::string>>);
	if (mysqlDbName.size() != mysqlFields.size())
	{
		return result;
	}
	m_redis.selectDbIndex(redisDbIndex);
	auto spResultSet = m_redis.getOrderGroupByScore(stock, useBeginTime.getDate(), useEndTime.getDate());
	if (spResultSet->toCount() == 0)
	{
		//如果没有实际数据同时有第一项则表示该gupiao没有有效数据，返回空
		if (!m_redis.getOrderGroupByScore(stock, 0, 0)->toGroup().empty())
		{
			return result;
		}

		std::vector<std::vector<std::vector<std::string>>> allDbData;
		//mysql移植到redis
		int32_t dbIndex = -1;
		while (dbIndex++ != mysqlDbName.size() - 1)
		{
			m_mysql.selectDb(mysqlDbName[dbIndex]);
			allDbData.push_back(m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::selectString(stock, mysqlFields[dbIndex], SqlString::fieldStrToDate("date") + ">=" + SqlString::strToDate(useBeginTime.dateToString()) + " and " + SqlString::fieldStrToDate("date") + " <= " + SqlString::strToDate(useEndTime.dateToString()))))->toVector());
		}
		
		std::vector<std::pair<int32_t, std::string>> orderGroup;
		dbIndex = -1;
		while (dbIndex++ != allDbData.size() - 1)
		{
			const std::vector<std::vector<std::string>>& dbResult = allDbData[dbIndex];
			int32_t lineIndex = -1;
			while (lineIndex++ != dbResult.size() - 1)
			{
				if (lineIndex == orderGroup.size())
				{
					orderGroup.push_back(std::pair<int32_t, std::string>());
				}
				std::string& strLine = orderGroup[lineIndex].second;
				const std::vector<std::string>& vecLine = dbResult[lineIndex];
				int32_t index = -1;
				while (index++ != vecLine.size() - 1)
				{
					strLine.append(vecLine[index] + ",");
				}
				if (dbIndex == allDbData.size() - 1)
				{
					strLine.pop_back();
				}
				if (dbIndex == 0)
				{
					orderGroup[lineIndex].first = IntDateTime(vecLine[0]).getDate();
				}
			}
		}
		orderGroup.push_back(std::pair<int32_t, std::string>(0, stock));
		
		if (!m_redis.setOrderGroups(stock, orderGroup))
		{
			RCSend("setOrderGroups error = %s, redisDbIndex = %d", stock.c_str(), redisDbIndex);
		}
		return result;
	}
	std::vector<std::string> vecLine;
	int32_t index = -1;
	while (index++ != spResultSet->toReply()->elements - 1)
	{
		CStringManager::split(vecLine, spResultSet->toReply()->element[index]->str, ',');
		result->push_back(vecLine);
	}
	return result;
}

std::map<std::string, std::vector<int32_t>> StockMysql::getIndex(const std::string& indexHashMapName) const
{
	m_redis.selectDbIndex(0);
	std::map<std::string, std::vector<int32_t>> result;
	auto hashMap = m_redis.getHashValues(indexHashMapName)->toHashMap();
	for (auto itIndex = hashMap.begin(); itIndex != hashMap.end(); ++itIndex)
	{
		std::vector<std::string> vecIndex;
		CStringManager::split(vecIndex, itIndex->second, ',');
		int32_t index = -1;
		while (index++ != vecIndex.size() - 1)
		{
			result[itIndex->first].push_back(atoi(vecIndex[index].c_str()));
		}
	}
	return result;
}

//int main()
//{
//	//MysqlCpp mysql;
//	//mysql.connect("127.0.0.1", 3306, "root", "");
//	//mysql.selectDb("stockmarket");
//	//IntDateTime beginTime = "2015-01-01";
//	//IntDateTime endTime = "2019-04-30";
//	std::shared_ptr<StockMysql> db = StockMysql::newCase();
//
//	db->initRedis();
//
//	//std::string stockTest = "603826";
//	//bool isExist = db->dateExist(stockTest, "2019-04-30");
//	//bool isExist1 = db->dateExist(stockTest, "2019-04-29");
//	//bool isExist2 = db->dateExist(stockTest, "2019-04-28");
//	//bool isExist3 = db->dateExist(stockTest, "2019-04-27");
//	//bool isExist4 = db->dateExist(stockTest, "2019-04-26");
//	//IntDateTime date1 = db->beginDate(stockTest);
//	//IntDateTime date2 = db->endDate(stockTest);
//	//IntDateTime date3 = db->getDatePre(stockTest, "2017-04-15");
//	//IntDateTime date4 = db->getDateNext(stockTest, "2017-04-15");
//	//BigNumber num5 = db->getDays(stockTest, "2017-04-01", "2019-05-09");
//
//	getchar();
//	return 0;
//}