#include "NetClientManager.h"
#include <stdint.h>
#include <stdio.h>
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProcessReceive.h"
#include "NetWork/NetWorkAPI.h"
#include "ClientManagerReceive.h"
#include "SharedMemory/SharedMemoryAPI.h"
#include "CDump/CDumpAPI.h"

SharedMemory* pid = nullptr;

BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close ConsoleTest");
		ProcessWork::instance().uninit();
		delete pid;
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

int32_t main()
{
	CDump::declareDumpFile();
	pid = SharedMemory::createPid();
	ClientManagerReceive clientManagerReceive;
	NetClient client;
	client.connect("106.12.204.167", 5203, &clientManagerReceive);//106.12.77.189 127.0.0.1
	ProcessReceive processReceive;
	processReceive.setNetClient(&client);
	ProcessWork::instance().initReceive(&processReceive);
	while (true) Sleep(1000);
	return 0;
}