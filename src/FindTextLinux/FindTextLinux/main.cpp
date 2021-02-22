#include "FindTextLinux.h"
#include <QtWidgets/QApplication>
#include <stdint.h>
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

extern uint32_t* g_searchPathThreadId;

#ifdef __unix__
#include <signal.h>
#include <stdlib.h>

//ctrl+c消息捕获函数
void CtrlCMessage(int eve)
{
	if (eve == 2)
	{
		//关闭退出事件
		//RCSend("close ConsoleTest");
		CTaskThreadManager::Instance().Uninit(*g_searchPathThreadId);
		*g_searchPathThreadId = 0;
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

int32_t main(int argc, char* argv[])
{
	QApplication app(argc, argv);
	FindTextLinux window;
	window.show();
	return app.exec();
}