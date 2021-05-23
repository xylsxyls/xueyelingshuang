#include "DtwsServer.h"
#include <stdint.h>
#include <stdio.h>
#ifdef __unix__
#include <signal.h>
#include <stdlib.h>
#endif
#include "NetSender/NetSenderAPI.h"
#include "ServerReceive.h"
#include "CSystem/CSystemAPI.h"

std::atomic<bool> g_exit = false;

#ifdef _MSC_VER
BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close DtwsServer");
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
		//RCSend("close DtwsServer");
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

#define DTWS_SERVER_VERSION "1.0"

int32_t main()
{
	ServerReceive receive;
	NetSender::instance().initServerReceive(&receive);
	ProcessWork::instance().initReceive();
	NetSender::instance().initServer(std::string("DtwsServer") + DTWS_SERVER_VERSION + " init");
	
	while (!g_exit)
	{
		CSystem::Sleep(1);
	}
	return 0;
}