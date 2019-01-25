#include "LogTest.h"
#include <stdint.h>
#include <stdio.h>
#include "LogReceive.h"
#include "NetSender/NetSenderAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close ConsoleTest");
		ProcessWork::instance().uninit();
		//CTaskThreadManager::Instance().UninitAll();
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

int32_t main()
{
	LogReceive logReceive;
	ProcessWork::instance().initReceive(&logReceive);
	NetSender::instance().init();
	printf("ComputerName = %s, pid = %d\n", CSystem::getComputerName().c_str(), CSystem::processPid());
	while (true) Sleep(1000);
	return 0;
}