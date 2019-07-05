#include "StockStrategyTest.h"
#include <stdint.h>
#include <stdio.h>
#include "StockStrategy/StockStrategyAPI.h"
#include "StockCondition/StockConditionAPI.h"
//#include "CDump/CDumpAPI.h"
#include "CStopWatch/CStopWatchAPI.h"
#include "CSystem/CSystemAPI.h"

BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close StockStrategyTest");
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

int32_t main()
{
	//CStopWatch watch1;
	//MysqlCpp m_mysql;
	//m_mysql.connect("127.0.0.1", 3306, "root", "");
	//m_mysql.selectDb("stockmarket");
	//std::vector<std::vector<std::string>> vecStock1 = m_mysql.execute(m_mysql.PreparedStatementCreator("select table_name from information_schema.tables where table_schema='stockmarket'"))->toVector();
	//std::string str;
	//int32_t index1 = -1;
	//while (index1++ != 1000 - 1)
	//{
	//	RCSend("market load = %d", index1 + 1);
	//	std::string& tableName = vecStock1[index1][0];
	//	if (tableName.size() != 6)
	//	{
	//		continue;
	//	}
	//	const std::string& stock = tableName;
	//	str.append(SqlString::selectString(stock) + " union ");
	//	//std::string name = m_mysql.execute(m_mysql.PreparedStatementCreator(SqlString::selectString("stock", "name", "stock='" + stock + "'")))->toVector()[0][0];
	//}
	//str.pop_back();
	//str.pop_back();
	//str.pop_back();
	//str.pop_back();
	//str.pop_back();
	//str.pop_back();
	//str.pop_back();
	//std::vector<std::vector<std::string>> vecMarket = m_mysql.execute(m_mysql.PreparedStatementCreator(str))->toVector();
	//RCSend("loadall = %d", watch1.GetWatchTime());
	//printf("loadall\n");
	//CDump::declareDumpFile();

	std::vector<std::string> vecStock;
	vecStock.push_back("000001");
	vecStock.push_back("000002");
	vecStock.push_back("000004");
	CStopWatch watch;
	StockAllMarket::instance().load();
	StockAllIndex::instance().load();
	RCSend("load = %d", watch.GetWatchTime());

	vecStock = StockAllMarket::instance().allStock();
	RCSend("allStock.size = %d", vecStock.size());
	//for (auto itStock = vecStock.begin(); itStock != vecStock.end(); )
	//{
	//	if (*itStock == "300274")
	//	{
	//		itStock = vecStock.erase(itStock);
	//		continue;
	//	}
	//	else if (*itStock == "603721")
	//	{
	//		itStock = vecStock.erase(itStock);
	//		continue;
	//	}
	//	else if (*itStock == "000681")
	//	{
	//		itStock = vecStock.erase(itStock);
	//		continue;
	//	}
	//	++itStock;
	//}

	IntDateTime beginTime = "2009-04-30";
	IntDateTime endTime = "2019-04-30";

	while (true)
	{
		int32_t temp = 0;
		printf("即将加载动态库\n");
		scanf("%d", &temp);
		CSystem::ClearScanf();

		StockConditionInterface::instance().load();

		StockFund stockFund;
		stockFund.add(100000);
		
		StockStrategy stockStrategy;
		stockStrategy.init(StockConditionInterface::instance().condition()->getStrategy(), &stockFund);
		watch.SetWatchTime(0);
		stockStrategy.profit(vecStock, beginTime, endTime);
		RCSend("runTime = %d", watch.GetWatchTime());
		stockFund.allFund(endTime);
		std::vector<std::string> stockLog = stockFund.stockLog();
		int32_t index = -1;
		while (index++ != stockLog.size() - 1)
		{
			//RCSend("%s", stockLog[index].c_str());
		}
		StockConditionInterface::instance().unload();
		printf("已卸载动态库\n");
	}
	
	getchar();
	return 0;
}