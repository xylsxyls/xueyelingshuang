#include "StockSave.h"
#include <stdint.h>
#include "StockMarket/StockMarketAPI.h"
#include "StockIndex/StockIndexAPI.h"
#include "CStopWatch/CStopWatchAPI.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"

BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close StockSave");
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

//在D盘下需要有所有股票的文件，第一列股票代码，第二列名字，第三列现价，需要先群体换\t\t为\t
void saveStock()
{
	MysqlCpp mysql;
	mysql.connect("127.0.0.1", 3306, "root", "");
	mysql.selectDb("stockmarket");
	Ctxt txt("D:\\stockall.txt");
	txt.LoadTxt(Ctxt::SPLIT, "\t");
	int32_t index = 0;
	while (index++ != txt.m_vectxt.size() - 2)
	{
		std::string stock = txt.m_vectxt[index][0];
		stock = CStringManager::Mid(stock, 2, 6);
		std::string& name = txt.m_vectxt[index][1];
		BigNumber price = txt.m_vectxt[index][2].c_str();
		if (price < 10)
		{
			continue;
		}
		if (CStringManager::Replace(name, "ST", "ST") != 0)
		{
			continue;
		}
		auto prepare = mysql.PreparedStatementCreator(SqlString::insertString("stock", "stock,name"));
		prepare->setString(0, stock);
		prepare->setString(1, name);
		mysql.execute(prepare);
	}
}

int32_t main()
{
	//saveStock();
	//return 0;
	
	//StockAllMarket::instance().save("D:\\stock");
	//StockAllIndex::instance().save();
	CStopWatch w;
	StockAllMarket::instance().load();
	StockAllIndex::instance().load();
	//auto sss = StockAllIndex::instance().allIndex();
	RCSend("time = %d, %s", w.GetWatchTime(), StockAllIndex::instance().index("002912")->rsi24("2019-04-30").toString().c_str());
	printf("end\n");
	getchar();
	return 0;
}