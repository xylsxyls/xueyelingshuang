#include "NetServerManager.h"
#include <stdint.h>
#include <stdio.h>
#include "Server.h"
#include "ServerProcessReceive.h"
#include <mutex>
#include "SharedMemory/SharedMemoryAPI.h"
#include "CDump/CDumpAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#ifdef __unix__
#include <signal.h>
#include <stdlib.h>
#endif

bool g_exit = false;

#ifdef _MSC_VER
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

#elif __unix__
//ctrl+c消息捕获函数
void CtrlCMessage(int eve)
{
	if (eve == 2)
	{
		//关闭退出事件
		//RCSend("close ConsoleTest");
		ProcessWork::instance().uninitReceive();
		g_exit = true;
		exit(0);
	}	
}

struct CtrlC
{
	CtrlC()
	{
		struct sigaction sigIntHandler;
		sigIntHandler.sa_handler = CtrlCMessage;
		sigemptyset(&sigIntHandler.sa_mask);
		sigIntHandler.sa_flags = 0;
		sigaction(SIGINT, &sigIntHandler, nullptr);
	}
};

CtrlC g_ctrlc;
#endif

int32_t main()
{
	CDump::declareDumpFile();

	ServerProcessReceive serverProcessReceive;
	ProcessWork::instance().initPostThread();
	ProcessWork::instance().addProcessReceiveCallback(&serverProcessReceive);
	ProcessWork::instance().initReceive();

	Server server;
	serverProcessReceive.setServer(&server);
	server.listen(5203);

	while (!g_exit)
	{
		CSystem::Sleep(1000);
	}
	return 0;
}