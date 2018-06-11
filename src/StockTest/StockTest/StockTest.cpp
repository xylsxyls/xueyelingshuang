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

int main()
{
	MysqlCpp mysql;
	bool isConnect = mysql.connect("127.0.0.1", 3306, "root", "");
	mysql.selectDb("test");
    Sleep(3000);
	CMouse::MoveAbsolute(xyls::Point(457, 1056));
	CMouse::LeftClick();
	Sleep(1500);
	std::vector<std::string> vecStock;
	vecStock.push_back("603045");
	vecStock.push_back("300718");
	vecStock.push_back("300702");
	vecStock.push_back("300720");
	vecStock.push_back("603709");
	vecStock.push_back("002927");
	vecStock.push_back("300682");
	vecStock.push_back("603013");
	std::map<std::string, std::string> useCountMap;
	Ctxt txt("D:\\stock.txt");
	txt.ClearFile();
	std::ofstream file("D:\\stock.txt");
	int32_t index = -1;
	while (index++ != vecStock.size() - 1)
	{
		txt.AddLine("%s", vecStock[index].c_str());
		Stock::getPriceFromScreen(vecStock[index]);
		Stock::insertDatabase(mysql);
		int32_t useCount = 0;
		auto map = Stock::getPriceMap(mysql, useCount);
		std::string& nowStr = useCountMap[CStringManager::Format("%d", useCount)];
		if (nowStr == "")
		{
			useCountMap[CStringManager::Format("%d", useCount)] = vecStock[index];
		}
		else
		{
			useCountMap[CStringManager::Format("%d", useCount)] = nowStr + ", " + vecStock[index];
		}
		useCountMap[CStringManager::Format("%d", useCount)] + " " + (vecStock[index]);
		CSystem::OutputVector(map, "D:\\stock.txt");
	}
	CSystem::OutputMap(useCountMap, "D:\\stockPriceMap.txt");
	CMouse::MoveAbsolute(xyls::Point(457, 1056));
	CMouse::LeftClick();
	Sleep(1500);
	printf("Íê³É\n");
	getchar();
	return 0;
}