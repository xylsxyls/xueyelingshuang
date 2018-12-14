#include "LogTestServer.h"
#include <stdint.h>
#include <stdio.h>
#include "ProcessClient/ProcessClientAPI.h"
#include "LogTestServerReceive.h"

BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close ConsoleTest");
		ProcessClient::instance().uninit();
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

int32_t main()
{
	LogTestServerReceive receive;
	ProcessClient::instance().initReceive(&receive);
	while (true) Sleep(1000);
	getchar();
	return 0;
}