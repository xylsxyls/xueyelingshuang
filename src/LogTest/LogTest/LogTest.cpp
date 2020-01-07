#include "LogTest.h"
#include <stdint.h>
#include <stdio.h>
#include "LogReceive.h"
#include "NetSender/NetSenderAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "CDump/CDumpAPI.h"
#include "LogManager/LogManagerAPI.h"

SharedMemory* pid = nullptr;

BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close ConsoleTest");
		ProcessWork::instance().uninit();
		delete pid;
		//CTaskThreadManager::Instance().UninitAll();
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

int32_t main()
{
	CDump::declareDumpFile();
	pid = SharedMemory::createPid();
	LogManager::instance().init();
	LogReceive logReceive;
	ProcessWork::instance().initReceive(&logReceive);
	NetSender::instance().init();
	printf("ComputerName = %s, pid = %d\n", CSystem::getComputerName().c_str(), CSystem::processFirstPid());
	while (true) Sleep(1000);
	return 0;
}