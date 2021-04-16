#include "BackStart.h"
#include <stdint.h>
#include <stdio.h>
#ifdef __unix__
#include <signal.h>
#include <stdlib.h>
#endif
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "Cini/CiniAPI.h"

#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")

#ifdef _MSC_VER
BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close BackStart");
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
		//RCSend("close BackStart");
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
	std::string currentExePath = CSystem::GetCurrentExePath();
	Cini ini(currentExePath + "BackStartConfig.ini");
	std::vector<std::string> vecStart = CStringManager::split(ini.readIni("start"), ",");
	int32_t index = -1;
	while (index++ != vecStart.size() - 1)
	{
		const std::string& exe = vecStart[index];
		std::vector<std::string> vecParam = CStringManager::split(exe, " ");
		if (vecParam.size() < 2)
		{
			continue;
		}
		if (vecParam[0] != "0")
		{
			CSystem::Sleep(atoi(vecParam[0].c_str()));
		}
		std::string param = CStringManager::Right(exe, exe.size() - (CStringManager::Find(exe, vecParam[1]) + vecParam[1].size()));
#ifdef _MSC_VER
		if (!param.empty())
		{
			param = CStringManager::Right(param, param.size() - 1);
		}
		ShellExecuteA(NULL, "open", (currentExePath + vecParam[1] + ".exe").c_str(), param.c_str(), NULL, SW_HIDE);
#elif __unix__
		CSystem::SystemCommand("start \"" + currentExePath + vecParam[1] + param + "\" &", true);
#endif
	}
	return 0;
}