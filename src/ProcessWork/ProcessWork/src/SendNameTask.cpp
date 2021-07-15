#include "SendNameTask.h"
#include "Semaphore/SemaphoreAPI.h"
#include "ProcessWork.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include <string.h>

SendNameTask::SendNameTask():
m_waitEndPostName(nullptr),
m_postNameQueue(nullptr),
m_postNameSemaphore(nullptr),
m_exit(false)
{

}

void SendNameTask::DoTask()
{
	while (!m_exit)
	{
		m_postNameSemaphore->wait();
		if (m_exit || (*m_waitEndPostName && m_postNameQueue->empty()))
		{
			break;
		}
		char* postNameBuffer = nullptr;
		m_postNameQueue->pop(&postNameBuffer);
		int32_t nameLength = strlen(postNameBuffer);

		ProcessWork::instance().send(postNameBuffer,
			(*((int32_t*)(postNameBuffer + nameLength + 1)) == 0) ? nullptr : (postNameBuffer + nameLength + 1 + 8),
			*((int32_t*)(postNameBuffer + nameLength + 1)),
			(MessageType)*((int32_t*)(postNameBuffer + nameLength + 1) + 1));

		delete[] postNameBuffer;
	}
}

void SendNameTask::StopTask()
{
	m_exit = true;
	m_postNameSemaphore->signal();
}

void SendNameTask::setParam(std::atomic<bool>* waitEndPostName, LockFreeQueue<char*>* postNameQueue, Semaphore* postNameSemaphore)
{
	m_waitEndPostName = waitEndPostName;
	m_postNameQueue = postNameQueue;
	m_postNameSemaphore = postNameSemaphore;
}