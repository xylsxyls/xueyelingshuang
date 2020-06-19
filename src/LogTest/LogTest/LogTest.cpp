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
#include "CorrespondParam/CorrespondParamAPI.h"

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

	LogManager::instance().init();

	LogReceive logReceive;
	ProcessWork::instance().initReceive(&logReceive);

	int32_t clientPid = CSystem::processFirstPid();
	ProtoMessage message;
	message[SERVER_NAME] = "LogTestServer1.0";
	NetSender::instance().init(message, CorrespondParam::ProtocolId::CLIENT_INIT);

	printf("ComputerName = %s, pid = %d\n", CSystem::getComputerName().c_str(), clientPid);

	while (!g_exit)
	{
		Sleep(1000);
	}
	return 0;
}