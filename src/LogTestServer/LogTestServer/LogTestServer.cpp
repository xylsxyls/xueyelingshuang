#include "LogTestServer.h"
#include <stdint.h>
#include <stdio.h>
#include "ProcessWork/ProcessWorkAPI.h"
#include "LogTestServerReceive.h"
#include "NetSender/NetSenderAPI.h"
#include "CDump/CDumpAPI.h"

bool g_exit = false;

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

int32_t main()
{
	CDump::declareDumpFile();

	LogTestServerReceive receive;
	ProcessWork::instance().initReceive(&receive);

	ProtoMessage message;
	NetSender::instance().init(message, CorrespondParam::ProtocolId::SERVER_INIT, true);

	while (!g_exit)
	{
		Sleep(1000);
	}
	return 0;
}