#include "ConsoleTest.h"
#include <stdint.h>
#include <stdio.h>
#ifdef __unix__
#include <signal.h>
#include <stdlib.h>
#endif

#ifdef _MSC_VER
BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//�ر��˳��¼�
		//RCSend("close ConsoleTest");
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
	
	getchar();
	return 0;
}