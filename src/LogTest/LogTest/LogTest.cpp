#include "LogTest.h"
#include <stdint.h>
#include <stdio.h>
#include "LogReceive.h"
#include "NetSender/NetSenderAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CDump/CDumpAPI.h"
#include "LogManager/LogManagerAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"

bool g_exit = false;

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

	printf("ComputerName = %s, pid = %d\n", CSystem::getComputerName().c_str(), CSystem::processFirstPid());

	while (!g_exit)
	{
		Sleep(1000);
	}
	return 0;
}