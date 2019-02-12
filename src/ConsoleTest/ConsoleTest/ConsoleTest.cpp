#include "ConsoleTest.h"
#include <stdint.h>
#include <stdio.h>
#include "LogSender/LogSenderAPI.h"
#include "LogManager/LogManagerAPI.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"

BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close ConsoleTest");
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

int32_t main()
{
	//RCSend("time = %d", ::GetTickCount());
	int32_t count = 25000;
	while (count-- != 0)
	{
		LOG_SEND_ALL("%d，%s", 123456, "我们");
		LOG_SEND_ALL("%d，%s", 1234567, "我们说");
		LOG_SEND_ALL("%d，%s", 12345678, "我们说来");
		LOG_SEND_ALL("%d，%s", 123456789, "我们说来了");
		if (count % 10000 == 0)
		{
			//RCSend("count last = %d", count);
		}
	}
	//RCSend("end,time = %d", ::GetTickCount());

	getchar();
	return 0;
}