#include "StockTongHuaShun.h"
#include <stdint.h>
#include <stdio.h>
#include "MysqlCpp/MysqlCppAPI.h"
#include "CStopWatch/CStopWatchAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "MairubishengTask.h"

BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close StockTongHuaShun");
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

int32_t main()
{
	printf("任意键3秒后开始\n");
	getchar();
	Sleep(3000);
	CStopWatch stopWatch;
	MysqlCpp mysql;
	bool ss = mysql.connect("127.0.0.1", 3306, "root", "");
	mysql.selectDb("stockmarket");
	auto allStock = mysql.execute(mysql.PreparedStatementCreator(SqlString::selectString("stock", "stock")))->toVector();
	auto threadId = CTaskThreadManager::Instance().Init();
	std::vector<std::string> vecValidStock;

	int32_t index = -1;
	while (index++ != allStock.size() - 1)
	{
		std::shared_ptr<MairubishengTask> spMairubishengTask;
		MairubishengTask* mairubishengTask = new MairubishengTask;
		mairubishengTask->setParam(allStock[index][0], &vecValidStock);
		spMairubishengTask.reset(mairubishengTask);
		CTaskThreadManager::Instance().GetThreadInterface(threadId)->PostTask(spMairubishengTask);
	}//"600354""600206"
	
	CTaskThreadManager::Instance().GetThreadInterface(threadId)->WaitForEnd();
	index = -1;
	while (index++ != vecValidStock.size() - 1)
	{
		LOG_SEND("%s", vecValidStock[index].c_str());
	}
	LOG_SEND("time = %d", stopWatch.GetWatchTime());
	
	getchar();
	getchar();
	return 0;
}