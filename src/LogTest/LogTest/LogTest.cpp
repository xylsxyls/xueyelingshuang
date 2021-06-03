#include "LogTest.h"
#include <stdint.h>
#include <stdio.h>
#include "LogReceive.h"
#include "NetSender/NetSenderAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CDump/CDumpAPI.h"
#include "LogManager/LogManagerAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "NetReceive.h"
#include "Semaphore/SemaphoreAPI.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "LogDeleteTask.h"
#ifdef __unix__
#include <signal.h>
#include <stdlib.h>
#endif

bool g_exit = false;

#ifdef _MSC_VER
BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close ConsoleTest");
		ProcessWork::instance().uninitPostThread();
		ProcessWork::instance().uninitReceive();
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
		ProcessWork::instance().uninitPostThread();
		ProcessWork::instance().uninitReceive();
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

#define LOGTEST_CLIENT_VERSION "1.1"
#define LOGTEST_SERVER_VERSION "1.1"

int32_t main()
{
	CDump::declareDumpFile();

	LogManager::instance().set(true, false);

	uint32_t screenThreadId = CTaskThreadManager::Instance().Init();
	uint32_t logThreadId = CTaskThreadManager::Instance().Init();
	uint32_t logDeleteThreadId = CTaskThreadManager::Instance().Init();
	uint32_t netThreadId = CTaskThreadManager::Instance().Init();

	std::shared_ptr<CTaskThread> spScreenThread = CTaskThreadManager::Instance().GetThreadInterface(screenThreadId);
	std::shared_ptr<CTaskThread> spLogThread = CTaskThreadManager::Instance().GetThreadInterface(logThreadId);
	std::shared_ptr<CTaskThread> spLogDeleteThread = CTaskThreadManager::Instance().GetThreadInterface(logDeleteThreadId);
	std::shared_ptr<CTaskThread> spNetThread = CTaskThreadManager::Instance().GetThreadInterface(netThreadId);

	std::atomic<int32_t> lastLogTime;
	lastLogTime = (int32_t)CSystem::GetTickCount();

	std::shared_ptr<LogDeleteTask> spLogDeleteTask(new LogDeleteTask);
	spLogDeleteTask->setParam(&lastLogTime, spLogThread);
	spLogDeleteThread->PostTask(spLogDeleteTask);

	Semaphore initResponseSemaphore;

	ProcessWork::instance().initPostThread();
	NetReceive netReceive;
	netReceive.setInitResponseSemaphore(&initResponseSemaphore);
	netReceive.setThread(spScreenThread, spLogThread);
	netReceive.setLastLogTime(&lastLogTime);
	NetSender::instance().initClientReceive(&netReceive);

	std::string loginName = CSystem::getComputerName();

	LogReceive logReceive;
	logReceive.setThread(spScreenThread, spLogThread, spNetThread);
	logReceive.setLastLogTime(&lastLogTime);
	logReceive.setLoginName(loginName);
	ProcessWork::instance().addProcessReceiveCallback(&logReceive);

	ProcessWork::instance().initReceive();

	printf("wait for server response ...\n");
	NetSender::instance().initClient(PROJECT_LOGTEST, std::string("LogTestServer") + LOGTEST_SERVER_VERSION, loginName);

	bool initResult = initResponseSemaphore.wait(1000);
	if (!initResult)
	{
		printf("cannot find NetClientManager\n");
	}
	printf("ComputerName = %s, pid = %d\n", CSystem::getComputerName().c_str(), CSystem::currentProcessPid());

	while (!g_exit)
	{
		CSystem::Sleep(1);
	}

	CTaskThreadManager::Instance().Uninit(screenThreadId);
	CTaskThreadManager::Instance().Uninit(logDeleteThreadId);
	CTaskThreadManager::Instance().Uninit(logThreadId);
	CTaskThreadManager::Instance().Uninit(netThreadId);
	return 0;
}