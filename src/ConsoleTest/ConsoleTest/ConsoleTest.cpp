#include "ConsoleTest.h"
#include <stdint.h>
#include "CMysql/CMysqlAPI.h"

int main()
{
	CMysql mysql;
	CTable* table = mysql.OpenTableInterface("testtable");
	CRecord record(table);
	record["field"] = "3";
	record["field2"] = 4;
	table->Add(&record);
	getchar();
	return 0;
}