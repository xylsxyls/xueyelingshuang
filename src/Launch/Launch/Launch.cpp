#include "Launch.h"
#include <stdint.h>
#include <stdio.h>
#ifdef __unix__
#include <signal.h>
#include <stdlib.h>
#endif
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"

#ifdef _MSC_VER
BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
		//RCSend("close Launch");
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
		//RCSend("close Launch");
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

int32_t main(int32_t argc, char** argv)
{
	std::string currentExePath = CSystem::GetCurrentExePath();
	std::string name = CSystem::inputString("请输入名称");
	std::string path;
	
	std::string commonFilePath = CSystem::commonFile(name);
	if (commonFilePath.empty())
	{
		path = CSystem::inputString("请输入可执行文件的绝对路径");
	}
	else
	{
		path = commonFilePath;
		name = CSystem::GetName(path, 1);
	}

	std::string content = "[Desktop Entry]\n"
	"Name={NAME}\n"
	"Comment={NAME}\n"
	"Exec={PATH}\n"
	"Terminal=false\n"
	"Type=Application\n"
	"Categories=Application;Development;";

	CStringManager::Replace(content, "{NAME}", name);
	CStringManager::Replace(content, "{PATH}", path);

	std::string launchLinkPath = "/home/" + CSystem::GetSysUserName() + "/" + name + ".desktop";
	CSystem::saveFile(content, launchLinkPath);

	std::string launchPath = "/usr/share/applications/" + name + ".desktop";

	std::string result;
	CSystem::SystemCommand("sudo cp " + launchLinkPath + " " + launchPath, result, true);
	CSystem::SystemCommand("rm " + launchLinkPath, result, true);
	printf("/usr/share/applications\n");
	return 0;
}