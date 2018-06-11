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

int main()
{
	MysqlCpp mysql;
	bool isConnect = mysql.connect("127.0.0.1", 3306, "root", "");
	mysql.selectDb("test");
    Sleep(3000);
    Stock::getPriceFromScreen("603045");
    Stock::insertDatabase(mysql);
    auto map = Stock::getPriceMap(mysql);
	CSystem::OutputVector(map,"D:\\stock.txt");
	getchar();
	return 0;
}