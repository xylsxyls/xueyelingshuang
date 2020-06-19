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
		ProcessWork::instance().uninitReceive();
		g_exit = true;
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

int32_t main()
{
	CDump::declareDumpFile();

	Client client;
	client.connect("106.12.204.167", 5203);//106.12.77.189 127.0.0.1 106.12.204.167

	ProcessReceive processReceive;
	ProcessWork::instance().initReceive(&processReceive);
	processReceive.setClient(&client);

	while (!g_exit)
	{
		Sleep(1000);
	}
	return 0;
}