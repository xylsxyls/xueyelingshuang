#include "MessageTestLinux.h"
#include <QtWidgets/QApplication>
#include <stdint.h>
#include "MessageTestLinux.h"
#include <stdlib.h>
#include <signal.h>
#include "CSystem/CSystemAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CDump/CDumpAPI.h"

//ctrl+c消息捕获函数
void CtrlCMessage(int eve)
{
	if (eve == 2)
	{
		//关闭退出事件
		//RCSend("close ConsoleTest");
		CSystem::deleteFile("/tmp/MessageTestLinux.file");
		CTaskThreadManager::Instance().UninitAll();
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

int32_t main(int argc, char* argv[])
{
	CDump::declareDumpFile();
	QApplication app(argc, argv);
	MessageTestLinux window;
	window.show();
	return app.exec();
}