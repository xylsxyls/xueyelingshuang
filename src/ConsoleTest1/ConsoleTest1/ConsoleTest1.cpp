#include "ConsoleTest1.h"
#include <stdint.h>
#include <stdio.h>

BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//�ر��˳��¼�
		//RCSend("close ConsoleTest1");
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

int32_t main()
{

	getchar();
	return 0;
}