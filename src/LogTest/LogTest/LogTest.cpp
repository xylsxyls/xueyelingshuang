#include "LogTest.h"
#include <stdint.h>
#include <stdio.h>
#include "LogReceive.h"
#include "ProcessClient/ProcessClientAPI.h"

BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//�ر��˳��¼�
		//RCSend("close ConsoleTest");
		ProcessClient::instance().uninit();
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

int32_t main()
{
	LogReceive logReceive;
	ProcessClient::instance().initReceive(&logReceive);
	ProcessClient::instance().send(nullptr, 0, "NetClientManager.exe", 0);
	while (true) Sleep(1000);
	return 0;
}