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
#include "LockFreeQueue/LockFreeQueueAPI.h"
#include "ScreenTask.h"
#include "LogTask.h"
#include "NetTask.h"
#include "ReadWriteMutex/ReadWriteMutexAPI.h"
#ifdef __unix__
#include <signal.h>
#include <stdlib.h>
#endif

bool g_exit = false;
uint32_t g_screenThreadId = 0;
uint32_t g_logThreadId = 0;
uint32_t g_logDeleteThreadId = 0;
uint32_t g_netThreadId = 0;

#ifdef _MSC_VER
BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close ConsoleTest");
		ProcessWork::instance().uninitReceive();
		
		CTaskThreadManager::Instance().Uninit(g_screenThreadId);
		CTaskThreadManager::Instance().Uninit(g_logDeleteThreadId);
		CTaskThreadManager::Instance().Uninit(g_logThreadId);
		CTaskThreadManager::Instance().Uninit(g_netThreadId);

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

#define LOGTEST_CLIENT_VERSION "1.2"
#define LOGTEST_SERVER_VERSION "1.1"

int32_t main()
{
	CDump::declareDumpFile();

	LogManager::instance().set(true, false);

	g_screenThreadId = CTaskThreadManager::Instance().Init();
	g_logThreadId = CTaskThreadManager::Instance().Init();
	g_logDeleteThreadId = CTaskThreadManager::Instance().Init();
	g_netThreadId = CTaskThreadManager::Instance().Init();

	ReadWriteMutex screenMutex;
	Semaphore screenSemaphore;
	LockFreeQueue<std::string> screenQueue;
	ReadWriteMutex logMutex;
	Semaphore logSemaphore;
	LockFreeQueue<std::string> logQueue;
	Semaphore netSemaphore;
	LockFreeQueue<std::string> netQueue;

	std::shared_ptr<ScreenTask> spScreenTask(new ScreenTask);
	spScreenTask->setParam(&screenSemaphore, &screenQueue);
	CTaskThreadManager::Instance().GetThreadInterface(g_screenThreadId)->PostTask(spScreenTask);

	std::shared_ptr<LogTask> spLogTask(new LogTask);
	spLogTask->setParam(&logSemaphore, &logQueue);
	CTaskThreadManager::Instance().GetThreadInterface(g_logThreadId)->PostTask(spLogTask);

	std::shared_ptr<NetTask> spNetTask(new NetTask);
	spNetTask->setParam(&netSemaphore, &netQueue);
	CTaskThreadManager::Instance().GetThreadInterface(g_netThreadId)->PostTask(spNetTask);

	std::atomic<int32_t> lastLogTime;
	lastLogTime = (int32_t)CSystem::GetTickCount();

	std::shared_ptr<LogDeleteTask> spLogDeleteTask(new LogDeleteTask);
	spLogDeleteTask->setParam(&logMutex, &logSemaphore, &logQueue, &lastLogTime);
	CTaskThreadManager::Instance().GetThreadInterface(g_logDeleteThreadId)->PostTask(spLogDeleteTask);

	Semaphore initResponseSemaphore;

	ProcessWork::instance().initPostThread();
	NetReceive netReceive;
	netReceive.setInitResponseSemaphore(&initResponseSemaphore);
	netReceive.setArea(&screenMutex, &screenSemaphore, &screenQueue, &logMutex, &logSemaphore, &logQueue);
	netReceive.setLastLogTime(&lastLogTime);
	NetSender::instance().initClientReceive(&netReceive);

	std::string loginName = CSystem::getComputerName();

	LogReceive logReceive;
	logReceive.setArea(&screenMutex, &screenSemaphore, &screenQueue, &logMutex, &logSemaphore, &logQueue, &netSemaphore, &netQueue);
	logReceive.setLastLogTime(&lastLogTime);
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

	CTaskThreadManager::Instance().Uninit(g_screenThreadId);
	CTaskThreadManager::Instance().Uninit(g_logDeleteThreadId);
	CTaskThreadManager::Instance().Uninit(g_logThreadId);
	CTaskThreadManager::Instance().Uninit(g_netThreadId);
	return 0;
}