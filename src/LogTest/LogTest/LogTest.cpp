#include "LogTest.h"
#include <stdint.h>
#include <stdio.h>
#include "LogReceive.h"
#include "NetSender/NetSenderAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CDump/CDumpAPI.h"
#include "LogManager/LogManagerAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
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
		NetSender::instance().uninitPostThread();
		NetSender::instance().uninitReceive();
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
		NetSender::instance().uninitPostThread();
		NetSender::instance().uninitReceive();
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

	LogManager::instance().init();

	NetSender::instance().initPostThread();
	LogReceive logReceive;
	NetSender::instance().initReceive(&logReceive);

	ProtoMessage message;
	message[SERVER_NAME] = "LogTestServer1.0";
	NetSender::instance().init(message, CorrespondParam::ProtocolId::CLIENT_INIT);

	printf("ComputerName = %s, pid = %d\n", CSystem::getComputerName().c_str(), CSystem::currentProcessPid());

	while (!g_exit)
	{
		CSystem::Sleep(1000);
	}
	return 0;
}