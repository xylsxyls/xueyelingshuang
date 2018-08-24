#include "SendTask.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "D:\\SendToMessageTest.h"
#include "CSystem/CSystemAPI.h"
#include "NetWorkThreadManager.h"

std::atomic<int> calc = 0;

SendTask::SendTask() :
m_text(nullptr),
m_libuvTcp(nullptr)
{

}

void SendTask::setLibuvTcp(LibuvTcp* libuvTcp)
{
	m_libuvTcp = libuvTcp;
}

void SendTask::setParam(char* text)
{
	m_text = text;
}

void SendTask::DoTask()
{
	++calc;
	m_libuvTcp->send(m_text);
	if (calc % 200000 == 0)
	{
		RCSend("client sendtask x = %d,time = %d,threadId = %d\n", calc, ::GetTickCount(), CSystem::SystemThreadId());
	}
}