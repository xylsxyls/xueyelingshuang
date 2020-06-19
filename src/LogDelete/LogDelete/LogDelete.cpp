#include "LogDelete.h"
#include <stdint.h>
#include "ProtoMessage/ProtoMessageAPI.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "CDump/CDumpAPI.h"
#include "CSystem/CSystemAPI.h"

BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//关闭退出事件
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

int32_t main()
{
	CDump::declareDumpFile();

	if (CSystem::deleteFile("LogTest1.0.exe.log"))
	{
		return 0;
	}

	ProtoMessage message;
	message[LOG_UNINIT] = (int32_t)true;
	std::string strMessage = message.toString();
	ProcessWork::instance().send("LogTest1.0", strMessage.c_str(), strMessage.length());
	return 0;
}