#include "LogTest.h"
#include <stdint.h>
#include <stdio.h>
#include "LogReceive.h"
#include "ProcessWork/ProcessWorkAPI.h"
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
	LogReceive logReceive;
	ProcessWork::instance().initReceive(&logReceive);
	NetSender::instance().send(nullptr, 0, 0);
	while (true) Sleep(1000);
	return 0;
}