#include "LogDelete.h"
#include <stdint.h>
#include <stdio.h>
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "CDump/CDumpAPI.h"

BOOL CALLBACK ConsoleHandler(DWORD eve)
{
	if (eve == CTRL_CLOSE_EVENT)
	{
		//�ر��˳��¼�
		//RCSend("close LogDelete");
	}
	return FALSE;
}

int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);

int32_t main()
{
	CDump::declareDumpFile();
	ProtoMessage message;
	message[LOG_UNINIT] = (int32_t)true;
	std::string strMessage = message.toString();
	ProcessWork::instance().send(strMessage.c_str(), strMessage.length(), "LogTest1.0.exe");
	return 0;
}