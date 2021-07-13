#include "NetTask.h"
#include "NetSender/NetSenderAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"

NetTask::NetTask():
m_netSemaphore(nullptr),
m_netQueue(nullptr),
m_exit(false)
{

}

void NetTask::DoTask()
{
	while (!m_exit)
	{
		m_netSemaphore->wait();
		if (m_exit)
		{
			return;
		}
		m_netQueue->pop(&m_buffer);
		NetSender::instance().postServer(PROJECT_LOGTEST, m_buffer);
	}
}

void NetTask::StopTask()
{
	m_exit = true;
	m_netSemaphore->signal();
}

void NetTask::setParam(Semaphore* netSemaphore, LockFreeQueue<std::string>* netQueue)
{
	m_netSemaphore = netSemaphore;
	m_netQueue = netQueue;
}