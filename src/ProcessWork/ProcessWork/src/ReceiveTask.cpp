#include "ReceiveTask.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ProcessReceiveCallback.h"
#include "Semaphore/SemaphoreAPI.h"

ReceiveTask::ReceiveTask():
m_callback(nullptr),
m_receiveQueue(nullptr),
m_receiveSemaphore(nullptr),
m_exit(false)
{
	
}

void ReceiveTask::DoTask()
{
	while (!m_exit)
	{
		m_receiveSemaphore->wait();
		if (m_exit)
		{
			break;
		}
		char* receive = nullptr;
		m_receiveQueue->pop(&receive);

		int32_t sendPid = *(int32_t*)receive;
		int32_t length = *((int32_t*)receive + 1) - 4;
		MessageType type = (MessageType)*((int32_t*)receive + 2);

		for (auto itCallback = m_callback->begin(); itCallback != m_callback->end(); ++itCallback)
		{
			(*itCallback)->receive(sendPid, receive + 12, length, type);
		}

		delete[] receive;
	}
}

void ReceiveTask::StopTask()
{
	m_exit = true;
	m_receiveSemaphore->signal();
}

void ReceiveTask::setParam(std::vector<ProcessReceiveCallback*>* callback,
	LockFreeQueue<char*>* receiveQueue,
	Semaphore* receiveSemaphore)
{
	m_callback = callback;
	m_receiveQueue = receiveQueue;
	m_receiveSemaphore = receiveSemaphore;
}