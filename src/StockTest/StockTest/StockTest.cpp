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
	int begin = ::GetTickCount();
	MysqlCpp mysql;
	bool isConnect = mysql.connect("127.0.0.1", 3306, "root", "");
	mysql.selectDb("test");
	//Stock::insertDatabase(mysql);
	auto sssds = Stock::getPriceMap(mysql);
	int end = ::GetTickCount();
	printf("%d", end - begin);
	getchar();
	return 0;
}