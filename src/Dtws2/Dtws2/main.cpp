#include "Dtws2.h"
#include <QtWidgets/QApplication>
#include <stdint.h>

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
	Dtws2 window;
	window.show();
	return app.exec();
}