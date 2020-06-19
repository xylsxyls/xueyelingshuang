#include "NetClientManager.h"
#include <stdint.h>
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProcessReceive.h"
#include "Client.h"
#include "CDump/CDumpAPI.h"

bool g_exit = false;

BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close ConsoleTest");
		ProcessWork::instance().uninitPostThread();
		ProcessWork::instance().uninitReceive();
		g_exit = true;
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

int32_t main()
{
	CDump::declareDumpFile();

	ProcessReceive processReceive;
	ProcessWork::instance().initPostThread();
	ProcessWork::instance().initReceive(&processReceive);

	Client client;
	processReceive.setClient(&client);

	client.connect("106.12.204.167", 5203);//106.12.77.189 127.0.0.1 106.12.204.167

	while (!g_exit)
	{
		Sleep(1000);
	}
	return 0;
}