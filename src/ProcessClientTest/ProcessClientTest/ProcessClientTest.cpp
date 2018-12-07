#include "ProcessClientTest.h"
#include <stdint.h>
#include <stdio.h>
#include "ProcessClient/ProcessClientAPI.h"
#include "CGetPath/CGetPathAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "LogManager/LogManagerAPI.h"

class Receive : public ReceiveCallback
{
public:
	virtual void receive(char* buffer, int32_t length, int32_t sendPid, int32_t protocalId)
	{
		//static int count = 0;
		//++count;
		//if (count % 1000000 == 0)
		//{
		//	RCSend("count = %d, time = %d", count, ::GetTickCount());
		//}
		static int recvTimes = 0;
		++recvTimes;
		LOGINFO("times = %d, %s.pid = %d, buffer = %s, length = %d, sendPid = %d, protocalId = %d", recvTimes, CGetPath::GetCurrentExeName().c_str(), CSystem::processPid(), buffer, length, sendPid, protocalId);


		//static int times = 10000;
		//--times;
		//if (times == -1)
		//{
		//	++times;
		//	return;
		//}

		std::string exeName = CStringManager::split(CGetPath::GetCurrentExeName(), "_")[0];
		int32_t currentId = atoi(CStringManager::split(CGetPath::GetCurrentExeName(), "_")[1].c_str());
		std::string sendText;
		int32_t count = currentId;
		while (count-- != 0)
		{
			sendText.append(CStringManager::Format("%d", currentId));
		}
		int pid = CSystem::processPid(CStringManager::Format("%s_%d.exe", exeName.c_str(), currentId + 1));
		//RCSend("recv exeName = %s, sendPid = %d, receivePid = %d", CGetPath::GetCurrentExeName().c_str(), CSystem::processPid(), pid);
		if (currentId + 1 <= 10)
		{
			ProcessClient::instance().send(sendText.c_str(), sendText.length(), CStringManager::Format("%s_%d.exe", exeName.c_str(), currentId + 1), currentId);
		}
	}
};

int32_t main()
{
	Receive receive;
	ProcessClient::instance().initReceive(&receive);

	Sleep(8000);
	//RCSend("%s.pid = %d", CGetPath::GetCurrentExeName().c_str(), CSystem::processPid());

	int sendTimes = 1000000;
	while (sendTimes-- != 0)
	{
		std::string exeName = CStringManager::split(CGetPath::GetCurrentExeName(), "_")[0];
		int32_t currentId = atoi(CStringManager::split(CGetPath::GetCurrentExeName(), "_")[1].c_str());
		std::string sendText;
		int32_t count = currentId;
		while (count-- != 0)
		{
			sendText.append(CStringManager::Format("%d", currentId));
		}
		int pid = CSystem::processPid(CStringManager::Format("%s_%d.exe", exeName.c_str(), currentId + 1));
		//RCSend("exeName = %s, sendPid = %d, receivePid = %d", CGetPath::GetCurrentExeName().c_str(), CSystem::processPid(), pid);
		if (currentId + 1 <= 10)
		{
			ProcessClient::instance().send(sendText.c_str(), sendText.length(), CStringManager::Format("%s_%d.exe", exeName.c_str(), currentId + 1), currentId);
		}
	}
	getchar();
	return 0;
}