#include "LogTestServer.h"
#include <stdint.h>
#include <stdio.h>
#include "ProcessWork/ProcessWorkAPI.h"
#include "LogTestServerReceive.h"
#include "NetSender/NetSenderAPI.h"

BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close ConsoleTest");
		ProcessWork::instance().uninit();
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

int32_t main()
{
	LogTestServerReceive receive;
	ProcessWork::instance().initReceive(&receive);
	NetSender::instance().init(nullptr, 0, true);
	while (true) Sleep(1000);
	getchar();
	return 0;
}