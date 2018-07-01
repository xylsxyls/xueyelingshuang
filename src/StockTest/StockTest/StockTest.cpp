#include "StockTest.h"
#include <stdint.h>
#include "MysqlCpp/MysqlCppAPI.h"
#include <vector>
#include "Ctxt/CtxtAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "BigNumber/BigNumberAPI.h"
#include <map>
#include "stock.h"
#include <windows.h>
#include "CSystem/CSystemAPI.h"
#include "CMouse/CMouseAPI.h"
#include <fstream>
#include "D:\\SendToMessageTest.h"
#include "IntDateTime/IntDateTimeAPI.h"
#include "CStopWatch/CStopWatchAPI.h"

int main()
{
	
	MysqlCpp mysql;
	bool isConnect = mysql.connect("192.168.1.2", 3306, "root", "");
	mysql.selectDb("stock");
    
	Stock::insertQuoteDataBase(mysql);
    Stock::chooseTest(mysql, Stock::getPreDate(mysql));
	return 0;

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
	int begin = ::GetTickCount();
    Sleep(2000);
	CMouse::MoveAbsolute(xyls::Point(457, 1056));
	CMouse::LeftClick();
	Sleep(1500);
    int32_t zubie = 1;
    std::vector<std::vector<std::string>> vecStock = Stock::getSelfStock(mysql, zubie);
	//std::vector<std::vector<std::string>> vecStock = Stock::getDefineStock("603721,600123,603100");
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

	int32_t index = -1;
    while (index++ != vecStock.size() - 1)//
	{
		getWatch.Run();
		txt.AddLine("%s", vecStock[index][0].c_str());
		Stock::getPriceFromScreen(vecStock[index][0]);
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
		//auto map = Stock::getPriceMap(mysql, useCount, reserveValue);
		auto map = Stock::getPriceMapFromLocal(useCount, reserveValue);
		priceMapWatch.Stop();

		printWatch.Run();
        Stock::printPriceMap(map);
		txt.AddLine("");
		printWatch.Stop();

		reserveMap[reserveValue].push_back(vecStock[index][0]);

        Stock::addChooseMap(chooseMap, map, vecStock[index][0]);

		std::string& nowStr = useCountMap[CStringManager::Format("%d", useCount)];
		if (nowStr == "")
		{
			useCountMap[CStringManager::Format("%d", useCount)] = vecStock[index][0];
		}
		else
		{
			useCountMap[CStringManager::Format("%d", useCount)] = nowStr + ", " + vecStock[index][0];
		}
		useCountMap[CStringManager::Format("%d", useCount)] + " " + (vecStock[index][0]);
	}
    Ctxt txtMap("D:\\stockPriceMap" + IntDateTime().dateToString() + ".txt");
	txtMap.ClearFile();
    CSystem::OutputMap(useCountMap, "D:\\stockPriceMap" + IntDateTime().dateToString() + ".txt");
	Stock::printReserveMap(reserveMap);
    Stock::printChooseMap(chooseMap);
    Stock::saveChooseToDataBase(mysql, chooseMap, zubie);
	CMouse::MoveAbsolute(xyls::Point(457, 1056));
	CMouse::LeftClick();
	Sleep(1000);
	int end = ::GetTickCount();
	printf("Íê³É£¬ºÄÊ±£º%dms\n", end - begin);
	printf("get = %dms,price = %dms,print = %dms", getWatch.GetWatchTime(), priceMapWatch.GetWatchTime(), printWatch.GetWatchTime());
	getchar();
	return 0;
}