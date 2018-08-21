#include "SendTask.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "D:\\SendToMessageTest.h"
#include "CSystem/CSystemAPI.h"
#include "NetWorkThreadManager.h"

std::atomic<int> calc = 0;

SendTask::SendTask() :
m_dest(nullptr),
m_buffer(nullptr),
m_length(0),
m_libuvTcp(nullptr)
{

}

void SendTask::setLibuvTcp(LibuvTcp* libuvTcp)
{
	m_libuvTcp = libuvTcp;
}

void SendTask::setParam(uv_tcp_t* dest, char* buffer, int32_t length)
{
	m_dest = dest;
	m_buffer = buffer;
	m_length = length;
}

void SendTask::DoTask()
{
	//if (!(m_libuvTcp->trySend(m_dest, m_buffer, m_length)))
	//{
	//	std::shared_ptr<SendTask> spTask;
	//	SendTask* sendTask = new SendTask;
	//	*sendTask = *this;
	//	spTask.reset(sendTask);
	//	NetWorkThreadManager::instance().postSendTaskToThreadPool(spTask, 2);
	//	return;
	//}
	m_libuvTcp->send(m_dest, m_buffer, m_length);
	if (m_length == 0)
	{
		return;
	}
	if (m_libuvTcp == nullptr)
	{
		RCSend("m_libuvTcp = nullptr error,calc = %d", calc);
	}
	::free(m_buffer);

	++calc;
	//m_libuvTcp->send(dest, text, length + 4);
	if (calc % 200000 == 0)
	{
		printf("x = %d,time = %d,threadId = %d\n", calc, ::GetTickCount(), CSystem::SystemThreadId());
	}
}