#include "StockStrategyTest.h"
#include <stdint.h>
#include <stdio.h>
#include "StockStrategy/StockStrategyAPI.h"
#include "StockCondition/StockConditionAPI.h"
#include "CDump/CDumpAPI.h"
#include "CStopWatch/CStopWatchAPI.h"

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
	CDump::declareDumpFile();
	CStopWatch watch;
	StockAllMarket::instance().load();
	StockAllIndex::instance().load();
	RCSend("load = %d", watch.GetWatchTime());

	StockFund stockFund;
	stockFund.add(100000);

	StockStrategy stockStrategy;
	stockStrategy.init(StockCondition::instance().getStrategy(), &stockFund);
	std::vector<std::string> vecStock = StockAllMarket::instance().allStock();
	for (auto itStock = vecStock.begin(); itStock != vecStock.end(); )
	{
		if (*itStock == "300274")
		{
			itStock = vecStock.erase(itStock);
			continue;
		}
		else if (*itStock == "603721")
		{
			itStock = vecStock.erase(itStock);
			continue;
		}
		++itStock;
	}
	//vecStock.push_back("000681");
	stockStrategy.run(vecStock, "2009-04-30", "2019-04-30");//StockAllMarket::instance().allStock()
	stockFund.allFund("2019-04-30");
	std::vector<std::string> stockLog = stockFund.stockLog();
	int32_t index = -1;
	while (index++ != stockLog.size() - 1)
	{
		RCSend("%s", stockLog[index].c_str());
	}
	getchar();
	return 0;
}