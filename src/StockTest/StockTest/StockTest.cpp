#include "StockTest.h"
#include "stock.h"
#include <stdint.h>
#include <vector>
#include <map>
#include <windows.h>
#include "MysqlCpp/MysqlCppAPI.h"
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CMouse/CMouseAPI.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "CStopWatch/CStopWatchAPI.h"
#include "Cini/CiniAPI.h"
#include "CGetPath/CGetPathAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include "D:\\SendToMessageTest.h"

int main()
{
	MysqlCpp mysql;
	Cini ini(CGetPath::GetCurrentExePath() + "config.ini");
	bool isConnect = mysql.connect(ini.readIni("ip"), 3306, "root", "");
	mysql.selectDb("stock");

    MysqlCpp mysqlfenbi;
	bool isConnectfenbi = mysqlfenbi.connect(ini.readIni("ip"), 3306, "root", "");
    mysqlfenbi.selectDb("stocktradequote");

	bool isGain = false;
    
    //回测，首先Table.txt中时今天行情
    //Stock::insertQuoteDataBase(mysql);
    //Stock::chooseTest(mysql, Stock::getPreDate(mysql));
    //return 0;

    //自选导入
    //int x = 3;
	//Ctxt self("D:\\Table.txt");
	//self.LoadTxt(2, "\t");
	//mysql.execute(mysql.PreparedStatementCreator(SqlString::clearTableString("selfstock")));
	//int32_t lineIndex = 0;
	//while (lineIndex++ != self.m_vectxt.size() - 2)
	//{
	//	auto lineStr = CStringManager::Mid(self.m_vectxt[lineIndex][0], 2, 6);
	//	auto state = mysql.PreparedStatementCreator(SqlString::insertString("selfstock", "daima"));
	//	state->setString(1, lineStr.c_str());
	//	mysql.execute(state);
	//}
	//return 0;

	//分笔存入
    //int beginfenbi = ::GetTickCount();
    //Sleep(2000);
    //xyls::Point tonghuashunPointfenbi(213, CSystem::GetWindowResolution().bottom - 23);
    //CMouse::MoveAbsolute(tonghuashunPointfenbi);
    //CMouse::LeftClick();
    //Sleep(1500);
    //std::vector<std::vector<std::string>> vecStockfenbi = Stock::getSelfStock(mysql);
    ////std::vector<std::vector<std::string>> vecStockfenbi = Stock::getDefineStock("603655");
    //int32_t indexfenbi = -1;
    //while (indexfenbi++ != vecStockfenbi.size() - 1)//
    //{
    //    Stock::getPriceFromScreen(vecStockfenbi[indexfenbi][0]);
    //    Stock::fenbiInsertDataBase(mysqlfenbi, vecStockfenbi[indexfenbi][0]);
    //}
    //CMouse::MoveAbsolute(tonghuashunPointfenbi);
    //CMouse::LeftClick();
    //int endfenbi = ::GetTickCount();
    //printf("完成，耗时：%dms\n", endfenbi - beginfenbi);
    //return 0;

	//分笔验证
    //std::string todayDate = IntDateTime().dateToString();
    //std::map<std::string, BigNumber> stockQuoteMap = Stock::getLastPriceFromStockQuote(mysql, todayDate);
    //std::map<std::string, BigNumber> stockTradeQuoteMap = Stock::getLastPriceFromStockTradeQuote(mysql, mysqlfenbi, todayDate);
    //for (auto itStock = stockQuoteMap.begin(); itStock != stockQuoteMap.end(); ++itStock)
    //{
    //    if (itStock->second != stockTradeQuoteMap.find(itStock->first)->second)
    //    {
    //        printf("%s\n", itStock->first.c_str());
    //    }
    //}
    //printf("完成");
    //getchar();
    //return 0;

	//最好分析，需要先存入分笔数据
	//Stock::bestAnalyzeDataBase(mysql, mysqlfenbi);
	//return 0;

	//估算
	//isGain = true;

	//正文
	int begin = ::GetTickCount();
    xyls::Point tonghuashunPoint(213, CSystem::GetWindowResolution().bottom - 23);
    if (!isGain)
    {
        Sleep(2000);
        CMouse::MoveAbsolute(tonghuashunPoint);
        CMouse::LeftClick();
        Sleep(1500);
    }
	int32_t zubie = atoi(ini.readIni("zubie").c_str());
    std::vector<std::vector<std::string>> vecStock = Stock::getSelfStock(mysql, isGain ? 0 : zubie);
	//std::vector<std::vector<std::string>> vecStock = Stock::getDefineStock("300584");
	std::map<std::string, std::string> useCountMap;
	std::map<BigNumber, std::vector<std::string>> reserveMap;
    std::map<BigNumber, std::vector<BigNumber>> chooseMap;
    Ctxt txt("D:\\stock" + IntDateTime().dateToString() + ".txt");
	txt.ClearFile();

	CStopWatch getWatch;
	getWatch.Stop();
	CStopWatch priceMapWatch;
	priceMapWatch.Stop();
	CStopWatch printWatch;
	printWatch.Stop();

    std::map<IntDateTime, std::map<std::string, std::vector<BigNumber>>> chooseMapAll;

	std::string strBeginTime;
	std::string strEndTime;
	if (isGain)
	{
		printf("beginTime = ");
        strBeginTime.resize(10);
		scanf("%s", &strBeginTime[0]);
		printf("endTime = ");
        strEndTime.resize(10);
        scanf("%s", &strEndTime[0]);
        printf("calc...\n");
        CSystem::ClearScanf();
	}

    IntDateTime beginTime;
    IntDateTime endTime;

	if (isGain)
	{
		beginTime.setTime(strBeginTime + " 00:00:00");
		endTime.setTime(strEndTime + " 00:00:00");
	}

    IntDateTime nowTime = beginTime;
    while (true)
    {
        if (nowTime > endTime)
        {
            break;
        }

        std::map<std::string, std::map<std::string, std::vector<BigNumber>>> capitalMapAll;

        int32_t index = -1;
        while (index++ != vecStock.size() - 1)//
        {
            getWatch.Run();
            txt.AddLine("%s", vecStock[index][0].c_str());
			if (!isGain)
			{
				Stock::getPriceFromScreen(vecStock[index][0]);
			}
            getWatch.Stop();

            priceMapWatch.Run();
            BigNumber reserveValue = 0;
            int32_t useCount = 0;
			std::map<std::string, std::vector<BigNumber>> map;
			if (isGain)
			{
				map = Stock::getCapitalMapFromDataBase(mysql, mysqlfenbi, vecStock[index][0], nowTime.dateToString());
			}
			else
			{
				map = Stock::getCapitalMapFromLocal(mysql, vecStock[index][0], nowTime.dateToString());
			}
            priceMapWatch.Stop();

            capitalMapAll[vecStock[index][0]] = map;

            printWatch.Run();
            Stock::printCapitalMap(map);
            txt.AddLine("");
            printWatch.Stop();
        }

        auto chooseVec = Stock::chooseFromCapitalMap(capitalMapAll);
        chooseMapAll[nowTime] = chooseVec;
        Stock::printChooseFromCapitalMap(chooseVec);
        txt.AddLine("");
        nowTime = nowTime + 86400;
    }

	if (isGain)
	{
		BigNumber gain = Stock::reckonGain(chooseMapAll);
		printf("gain = %s\n", gain.toString().c_str());
		RCSend("gain = %s", gain.toString().c_str());
	}
	else
	{
		Stock::saveCapitalChooseToDataBase(mysql, chooseMapAll, zubie);
		Stock::deleteCapitalChooseToDataBase(mysql);
	}
	
    if (!isGain)
    {
        CMouse::MoveAbsolute(tonghuashunPoint);
        CMouse::LeftClick();
        Sleep(1000);
    }
	int end = ::GetTickCount();
	printf("完成，耗时：%dms\n", end - begin);
	printf("get = %dms,price = %dms,print = %dms", getWatch.GetWatchTime(), priceMapWatch.GetWatchTime(), printWatch.GetWatchTime());
	getchar();
	return 0;
}