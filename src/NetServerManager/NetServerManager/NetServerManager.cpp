#include "NetServerManager.h"
#include <stdint.h>
#include <stdio.h>
#include "ServerManagerReceive.h"
#include "ServerManagerProcessReceive.h"
#include <mutex>
#include "SharedMemory/SharedMemoryAPI.h"

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
	pid = SharedMemory::createPid();

	ServerManagerReceive serverManagerReceive;

	NetServer server;
	server.listen(5203, &serverManagerReceive);

	ServerManagerProcessReceive serverManagerProcessReceive;
	serverManagerProcessReceive.setServer(&server);
	ProcessWork::instance().initReceive(&serverManagerProcessReceive);

	while (true) Sleep(1000);
	return 0;
}