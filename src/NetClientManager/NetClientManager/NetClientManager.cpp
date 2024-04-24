#include "NetClientManager.h"
#include <stdint.h>
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProcessReceive.h"
#include "Client.h"
#include "CDump/CDumpAPI.h"
#include "CSystem/CSystemAPI.h"
#ifdef __unix__
#include <signal.h>
#include <stdlib.h>
#endif

bool g_exit = false;
Client* g_client = nullptr;

#ifdef _MSC_VER
BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close ConsoleTest");
		ProcessWork::instance().uninitReceive();
		if (g_client != nullptr)
		{
			g_client->close();
		}
		g_exit = true;
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
		//RCSend("close ConsoleTest");
		ProcessWork::instance().uninitReceive();
		if (g_client != nullptr)
		{
			g_client->close();
		}
		g_exit = true;
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

std::string g_ip = "127.0.0.1";//106.12.204.167 49.235.147.184
int32_t g_port = 5203;

int main(int argc, char** argv)
{
	std::vector<std::string> vecParam = CSystem::exeParam(argc, argv);
	if (vecParam.size() >= 2)
	{
		g_ip = vecParam[1];
	}
	CDump::declareDumpFile();

	ProcessReceive processReceive;
	ProcessWork::instance().initPostThread();
	ProcessWork::instance().addProcessReceiveCallback(&processReceive);
	ProcessWork::instance().initReceive();

	Client client;
	g_client = &client;
	processReceive.setClient(&client);

	client.setFirstMessageLength(16);
	client.connect(g_ip.c_str(), g_port);//106.12.77.189 127.0.0.1 106.12.204.167

	while (!g_exit)
	{
		CSystem::Sleep(1000);
	}
	return 0;
}
