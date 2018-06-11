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
	mysql.selectDb("stock");
    Sleep(3000);
	CMouse::MoveAbsolute(xyls::Point(457, 1056));
	CMouse::LeftClick();
	Sleep(1500);
	std::vector<std::vector<std::string>> vecStock = Stock::getSelfStock(mysql);
	std::map<std::string, std::string> useCountMap;
	Ctxt txt("D:\\stock.txt");
	txt.ClearFile();
	int32_t index = -1;
	while (index++ != vecStock.size() - 1)
	{
		txt.AddLine("%s", vecStock[index][0].c_str());
		Stock::getPriceFromScreen(vecStock[index][0]);
		Stock::insertDatabase(mysql);
		int32_t useCount = 0;
		auto map = Stock::getPriceMap(mysql, useCount);
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
		CSystem::OutputVector(map, "D:\\stock.txt");
		txt.AddLine("");
	}
	CSystem::OutputMap(useCountMap, "D:\\stockPriceMap.txt");
	CMouse::MoveAbsolute(xyls::Point(457, 1056));
	CMouse::LeftClick();
	Sleep(1500);
	printf("Íê³É\n");
	getchar();
	return 0;
}