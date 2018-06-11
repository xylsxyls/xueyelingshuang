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
	int32_t index = -1;
	while (index++ != vecStock.size() - 1)
	{
		std::ofstream file("D:\\stock.txt");
		file << vecStock[index].c_str() << "\r\n";
		Stock::getPriceFromScreen(vecStock[index]);
		Stock::insertDatabase(mysql);
		auto map = Stock::getPriceMap(mysql);
		CSystem::OutputVector(map, "D:\\stock.txt");
	}
	CMouse::MoveAbsolute(xyls::Point(457, 1056));
	CMouse::LeftClick();
	Sleep(1500);
	printf("Íê³É\n");
	getchar();
	return 0;
}