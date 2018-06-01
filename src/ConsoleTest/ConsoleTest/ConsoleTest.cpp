#include "ConsoleTest.h"
#include <stdint.h>
#include "MysqlCpp/MysqlCppAPI.h"

int main()
{
	MysqlCpp mysql;
	bool isConnect = mysql.connect("127.0.0.1", 3306, "root", "");
	mysql.selectDb("test");
	auto statement = mysql.PreparedStatementCreator(SqlString::selectString("testtable", "id=3","num"));
	//statement->setInt(1, 80);
	//statement->setString(2, "aaa");
	//statement->setDouble(3, 808.22);
	std::shared_ptr<MysqlCppResultSet> result = mysql.executeResult(statement);
	while (result->next())
	{
		double id = result->getDouble("num");
		id = id;
	}
	getchar();
	return 0;
}