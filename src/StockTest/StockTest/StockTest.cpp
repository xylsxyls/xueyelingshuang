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
	bool isConnect = mysql.connect(ini.ReadIni("ip"), 3306, "root", "");
	mysql.selectDb("stock");

    MysqlCpp mysqlfenbi;
    bool isConnectfenbi = mysqlfenbi.connect(ini.ReadIni("ip"), 3306, "root", "");
    mysqlfenbi.selectDb("stocktradequote");
    
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
	//MysqlCpp mysqlfenbi;
	//bool isConnectfenbi = mysqlfenbi.connect(ini.ReadIni("ip"), 3306, "root", "");
	//mysqlfenbi.selectDb("stocktradequote");
    //
    //int beginfenbi = ::GetTickCount();
    //Sleep(2000);
    //xyls::Point tonghuashunPointfenbi(213, CSystem::GetWindowResolution().bottom - 23);
    //CMouse::MoveAbsolute(tonghuashunPointfenbi);
    //CMouse::LeftClick();
    //Sleep(1500);
    //std::vector<std::vector<std::string>> vecStockfenbi = Stock::getSelfStock(mysql);
    ////std::vector<std::vector<std::string>> vecStockfenbi = Stock::getDefineStock("603721,603100");
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
    //MysqlCpp mysqlfenbi;
    //bool isConnectfenbi = mysqlfenbi.connect(ini.ReadIni("ip"), 3306, "root", "");
    //mysqlfenbi.selectDb("stocktradequote");
    //std::string todayDate = IntDateTime().dateToString();
    //std::map<std::string, BigNumber> stockQuoteMap = Stock::getLastPriceFromStockQuote(mysql, todayDate);
    //std::map<std::string, BigNumber> stockTradeQuoteMap = Stock::getLastPriceFromStockTradeQuote(mysql, mysqlfenbi, todayDate);
    //for (auto itStock = stockQuoteMap.begin(); itStock != stockQuoteMap.end(); ++itStock)
    //{
    //	if (itStock->second != stockTradeQuoteMap.find(itStock->first)->second)
    //	{
    //		printf("%s\n", itStock->first.c_str());
    //	}
    //}
    //printf("完成");
    //getchar();
    //return 0;

	//最好分析，需要先存入分笔数据
	//Stock::bestAnalyzeDataBase(mysql, mysqlfenbi);
	//return 0;

	//正文
	int begin = ::GetTickCount();
    Sleep(2000);
    xyls::Point tonghuashunPoint(213, CSystem::GetWindowResolution().bottom - 23);
    CMouse::MoveAbsolute(tonghuashunPoint);
	CMouse::LeftClick();
	Sleep(1500);
    int32_t zubie = atoi(ini.ReadIni("zubie").c_str());
    std::vector<std::vector<std::string>> vecStock = Stock::getSelfStock(mysql, 0);
	//std::vector<std::vector<std::string>> vecStock = Stock::getDefineStock("603721,002799,002847");
	std::map<std::string, std::string> useCountMap;
	std::map<BigNumber, std::vector<std::string>> reserveMap;
    std::map<BigNumber, std::vector<BigNumber>> chooseMap;
    Ctxt txt("D:\\stock" + IntDateTime().dateToString() + ".txt");
	txt.ClearFile();
    Ctxt txtError("D:\\stockError" + IntDateTime().dateToString() + ".txt");
	txtError.ClearFile();

	CStopWatch getWatch;
	getWatch.Stop();
	//CStopWatch insertWatch;
	//insertWatch.Stop();
	CStopWatch priceMapWatch;
	priceMapWatch.Stop();
	CStopWatch printWatch;
	printWatch.Stop();

	std::map<std::string, std::map<std::string, std::vector<BigNumber>>> capitalMapAll;

	int32_t index = -1;
    while (index++ != vecStock.size() - 1)//
	{
		getWatch.Run();
		txt.AddLine("%s", vecStock[index][0].c_str());
		//Stock::getPriceFromScreen(vecStock[index][0]);
		getWatch.Stop();

		//insertWatch.Run();
		//bool insertSuccess = Stock::insertDatabase(mysql);
		//if (insertSuccess == false)
		//{
		//	txtError.AddLine("%s", vecStock[index][0].c_str());
		//}
		//insertWatch.Stop();

		priceMapWatch.Run();
		BigNumber reserveValue = 0;
		int32_t useCount = 0;
        //auto map = Stock::getPriceMapFromDataBase(mysqlfenbi, vecStock[index][0], useCount, reserveValue, "2018-07-09");
		auto map = Stock::getCapitalMapFromDataBase(mysql, mysqlfenbi, vecStock[index][0], "2018-07-05");
		//auto map = Stock::getPriceMapFromLocal(useCount, reserveValue);
		priceMapWatch.Stop();

		capitalMapAll[vecStock[index][0]] = map;

		printWatch.Run();
        //Stock::printPriceMap(map);
		Stock::printCapitalMap(map);
		txt.AddLine("");
		printWatch.Stop();

		//reserveMap[reserveValue].push_back(vecStock[index][0]);

		//Stock::addChooseMap(chooseMap, map, vecStock[index][0]);
		//
		//std::string& nowStr = useCountMap[CStringManager::Format("%d", useCount)];
		//if (nowStr == "")
		//{
		//	useCountMap[CStringManager::Format("%d", useCount)] = vecStock[index][0];
		//}
		//else
		//{
		//	useCountMap[CStringManager::Format("%d", useCount)] = nowStr + ", " + vecStock[index][0];
		//}
		//useCountMap[CStringManager::Format("%d", useCount)] + " " + (vecStock[index][0]);
	}

	auto chooseVec = Stock::chooseFromCapitalMap(capitalMapAll);
	RCSend("size = %d", chooseVec.size() - 1);
	RCSend("zuigao = %s,zuizhong = %s", chooseVec["888888"][0].toString().c_str(), chooseVec["888888"][1].toString().c_str());
	RCSend("zuigaobaifenbi = %s,zuizhongbaifenbi = %s", chooseVec["888888"][2].toString().c_str(), chooseVec["888888"][3].toString().c_str());

    //Ctxt txtMap("D:\\stockPriceMap" + IntDateTime().dateToString() + ".txt");
	//txtMap.ClearFile();
    //CSystem::OutputMap(useCountMap, "D:\\stockPriceMap" + IntDateTime().dateToString() + ".txt");
	//Stock::printReserveMap(reserveMap);
	//Stock::printChooseMap(chooseMap);
	//Stock::saveChooseToDataBase(mysql, chooseMap, zubie);
    CMouse::MoveAbsolute(tonghuashunPoint);
	CMouse::LeftClick();
	Sleep(1000);
	int end = ::GetTickCount();
	printf("完成，耗时：%dms\n", end - begin);
	printf("get = %dms,price = %dms,print = %dms", getWatch.GetWatchTime(), priceMapWatch.GetWatchTime(), printWatch.GetWatchTime());
	getchar();
	return 0;
}