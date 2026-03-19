#include "TransCharacter.h"
#include <stdint.h>
#include <stdio.h>
#ifdef __unix__
#include <signal.h>
#include <stdlib.h>
#endif
#include "CMouse/CMouseAPI.h"

#ifdef _MSC_VER
BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close TransCharacter");
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
		//RCSend("close TransCharacter");
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
	while (true)
	{
		printf("请输入转换循环次数:");
		int times = 0;
		scanf("%d", &times);
		while (times-- != 0)
		{
			CMouse::MoveClick(xyls::Point(243, 31), 100);
			CMouse::MoveClick(xyls::Point(226, 240), 100);
			CMouse::MoveClick(xyls::Point(59, 52), 100);
			CMouse::MoveAbsolute(xyls::Point(24, 81), 100);
			CMouse::RightClick(0);
			CMouse::MoveClick(xyls::Point(32, 90), 100);
		}
	}
	getchar();
	return 0;
}