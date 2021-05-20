#include "LogTestServer.h"
#include <stdint.h>
#include "NetSender/NetSenderAPI.h"
#include "LogTestServerReceive.h"
#include "NetSender/NetSenderAPI.h"
#include "CDump/CDumpAPI.h"
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
		//�ر��˳��¼�
		//RCSend("close ConsoleTest");
		ProcessWork::instance().uninitPostThread();
		ProcessWork::instance().uninitReceive();
		g_exit = true;
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

#elif __unix__
//ctrl+c��Ϣ������
void CtrlCMessage(int eve)
{
	if (eve == 2)
	{
		//�ر��˳��¼�
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

#define LOGTEST_SERVER_VERSION "1.1"

int32_t main()
{
	CDump::declareDumpFile();

	LogTestServerReceive receive;
	ProcessWork::instance().initPostThread();
	NetSender::instance().initServerReceive(&receive);
	ProcessWork::instance().initReceive();

	NetSender::instance().initServer(std::string("LogTestServer") + LOGTEST_SERVER_VERSION + " init");

	while (!g_exit)
	{
		CSystem::Sleep(1000);
	}
	return 0;
}