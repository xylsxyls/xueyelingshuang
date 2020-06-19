#include "NetServerManager.h"
#include <stdint.h>
#include <stdio.h>
#include "Server.h"
#include "ServerProcessReceive.h"
#include <mutex>
#include "SharedMemory/SharedMemoryAPI.h"
#include "CDump/CDumpAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

bool g_exit = false;

BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close ConsoleTest");
		ProcessWork::instance().uninitPostThread();
		ProcessWork::instance().uninitReceive();
		g_exit = false;
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

int32_t main()
{
	CDump::declareDumpFile();

	ServerProcessReceive serverProcessReceive;
	ProcessWork::instance().initPostThread();
	ProcessWork::instance().initReceive(&serverProcessReceive);

	Server server;
	serverProcessReceive.setServer(&server);
	server.listen(5203);

	while (!g_exit)
	{
		Sleep(1000);
	}
	return 0;
}