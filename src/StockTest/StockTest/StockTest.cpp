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

int main()
{
	MysqlCpp mysql;
	bool isConnect = mysql.connect("127.0.0.1", 3306, "root", "");
	mysql.selectDb("test");
    Sleep(3000);
    Stock::getPriceFromScreen("000001");
    Stock::insertDatabase(mysql);
    Stock::getPriceMap(mysql);
	getchar();
	return 0;
}