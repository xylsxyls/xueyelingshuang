#include "ConsoleTest.h"
#include <stdint.h>
#include "MysqlCpp/MysqlCppAPI.h"
#include <vector>

int main()
{
	MysqlCpp mysql;
	bool isConnect = mysql.connect("127.0.0.1", 3306, "root", "");
	mysql.selectDb("test");
	std::vector<std::string> vecField;
	vecField.push_back("num int(11)");
	vecField.push_back("data varchar(20)");
	auto statement = mysql.PreparedStatementCreator(SqlString::createDatabase("test2table"));
	//statement->setInt(1, 80);
	//statement->setString(2, "aaa");
	//statement->setDouble(3, 808.22);
	std::shared_ptr<MysqlCppResultSet> result = mysql.execute(statement);
	while (result->next())
	{
		double id = result->getDouble("num");
		id = id;
	}
	getchar();
	return 0;
}